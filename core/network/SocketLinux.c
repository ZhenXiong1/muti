/*
 * Socket.c
 *
 *  Created on: 2020\12\9
 *      Author: Rick
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "network/Socket.h"

#if defined(WIN32)

typedef struct SocketPrivate {
        SocketLinuxParam     param;
} SocketPrivate;

static void destroy(Socket* obj) {
        SocketPrivate *priv_p = obj->p;
        free(priv_p);
}

static SocketMethod method = {
        .destroy = destroy,
};

bool initSocketLinux(Socket* obj, SocketLinuxParam* param) {
	SocketPrivate *priv_p = malloc(sizeof(*priv_p));

	obj->p = priv_p;
	obj->m = &method;
	memcpy(&priv_p->param, param, sizeof(*param));
        

	return true;
}

#else

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include "ConnectionLinux.h"
#include "Log.h"

//#define DLOG_SOCKET(str, args...) DLOG(str, ##args)
#define DLOG_SOCKET(str, args...)

#define MAXEVENTS 128

typedef struct SocketPrivate {
        SocketLinuxParam        param;
        SocketIPType            ip_type;
        int                     sfd;
        int                     cfd;
        int                     efd;
        pthread_t               epoll_tid;
        bool                    stop;
        pthread_spinlock_t      conn_lock;
        ListHead                conn_head;
} SocketPrivate;


static inline void socketIODone(SocketPrivate *priv_p, ConnectionLinux *conn_p, char *where) {
        int left = __sync_sub_and_fetch(&conn_p->ref, 1);
        DLOG_SOCKET("socketIODone:%d(%p), %s", left, conn_p, where);
        if (left == 0) {
                DLOG_SOCKET("Release connection:%p", conn_p);
                pthread_spin_lock(&priv_p->conn_lock);
                listDel(&conn_p->element);
                pthread_spin_unlock(&priv_p->conn_lock);
                close(conn_p->fd);
                ((ConnectionLinuxMethod*)conn_p->super.m)->destroy(&conn_p->super);
                free(conn_p);
        }
        assert(left >= 0);
}


static inline void socketReadDoneCallback(IOContext *ioctx) {
        ConnectionLinux *conn_p = containerOf(ioctx->conn, ConnectionLinux, super);
        Socket *socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;

        ioctx->readCallback(ioctx->conn, ioctx->rc, ioctx->buf, ioctx->trans_num, ioctx->cb_arg);
        socketIODone(priv_p, conn_p, "socketIoDoneCallback"); // subtract conn->read/write add ref
        free(ioctx);
}

static inline void socketWriteDoneCallback(IOContext *ioctx) {
        ConnectionLinux *conn_p = containerOf(ioctx->conn, ConnectionLinux, super);
        Socket *socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;

        ioctx->writeCallback(ioctx->conn, ioctx->rc, ioctx->cb_arg);
        socketIODone(priv_p, conn_p, "socketIoDoneCallback"); // subtract conn->read/write add ref
        free(ioctx);
}

static void socketPostIOJobs(ConnectionLinux *conn_p, SocketPrivate *priv_p) {
        ListHead head;
        IOContext *ioctx, *ioctx1;

//        sleep(1);
        listHeadInit(&head);
        pthread_spin_lock(&conn_p->read_jobs_head_lck);
        listJoinTailInit(&conn_p->read_jobs_head, &head);
        pthread_spin_unlock(&conn_p->read_jobs_head_lck);

        listForEachEntrySafe(ioctx, ioctx1, &head, element) {
                ioctx->trans_num = 0;
                listDel(&ioctx->element);
                ioctx->rc = false;
                socketReadDoneCallback(ioctx);
        }

        pthread_spin_lock(&conn_p->write_jobs_head_lck);
        listJoinTailInit(&conn_p->write_jobs_head, &head);
        pthread_spin_unlock(&conn_p->write_jobs_head_lck);

        listForEachEntrySafe(ioctx, ioctx1, &head, element) {
                ioctx->trans_num = 0;
                listDel(&ioctx->element);
                ioctx->rc = false;
                socketWriteDoneCallback(ioctx);
        }


}

void socketCloseConnection(ConnectionLinux *conn_p) {
        Socket* socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;

        int is_closed = __sync_add_and_fetch(&conn_p->is_closed, 1);
        DLOG_SOCKET("CLOSE Connection, is_closed:%d", is_closed);
        if (is_closed == 1) {
                socketPostIOJobs(conn_p, priv_p);
                priv_p->param.super.onClose(&conn_p->super);
                socketIODone(priv_p, conn_p, "socketCloseConnection");
        }
}

static int makeSocketNonBlocking(int sfd) {
        int flags, s;
        flags = fcntl(sfd, F_GETFL,0);
        if(flags == -1) {
                ELOG("fcntl get error");
                return-1;
        }

        flags|= O_NONBLOCK;
        s =fcntl(sfd, F_SETFL, flags);
        if(s ==-1) {
                ELOG("fcntl set error");
                return-1;
        }
        return 0;
}

static ssize_t socketNoneBlockRead(int sfd, void *buf, size_t len) {
        ssize_t nread = 0, n, to_read = len;
        char *p = buf;

        while (to_read) {
                n = read(sfd, p, to_read);
                if (n == 0) {
                	ELOG("read error 1: read 0 byte, remote close");
                        nread = -1; // the other side closed, return -1 to let the caller know the error
                        break;
                }
                if (n == -1) {
                        if (errno == EINTR)
                                continue; // interrupted, read again
                        else if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break; // EAGAIN/EWOULDBLOCK, return >=0 to let the caller treat it as success
                        ELOG("read error 2: read errno:%d", errno);
                        nread = -1;
                        break;
                }
                nread += n;
                to_read -= n;
                p += n;
        }
        return nread;
}

static void socketDoRead(Job *job) {
        ConnectionLinux *conn_p = containerOf(job, ConnectionLinux, read_job);
        Socket *socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;
        IOContext *ioctx, *ioctx1;
        ListHead read_head;

        listHeadInit(&read_head);
        __sync_sub_and_fetch(&conn_p->is_reading, 1);
        pthread_spin_lock(&conn_p->read_jobs_head_lck);
        listJoinTailInit(&conn_p->read_jobs_head, &read_head);
        pthread_spin_unlock(&conn_p->read_jobs_head_lck);

        if (conn_p->is_closed) {
                goto closed;
        }

        listForEachEntrySafe(ioctx, ioctx1, &read_head, element) {
                ssize_t nread = socketNoneBlockRead(conn_p->fd, ioctx->buf, ioctx->buf_size);
                if (nread == -1) {
                	conn_p->super.m->close(&conn_p->super);
                        goto closed;
                } else if (nread == 0) {
                        break;
                } else {
                        ioctx->trans_num = nread;
                        listDel(&ioctx->element);
                        ioctx->rc = true;

                        size_t buf_size = ioctx->buf_size;
                        socketReadDoneCallback(ioctx);
                        if (nread < buf_size) {
                                break;
                        }
                }
        }
        if (!listEmpty(&read_head)) {
                pthread_spin_lock(&conn_p->read_jobs_head_lck);
                listJoinInit(&read_head, &conn_p->read_jobs_head);
                pthread_spin_unlock(&conn_p->read_jobs_head_lck);
        }
        socketIODone(priv_p, conn_p, "socketDoRead1");   // subtract tp add ref
        return;
closed:
        listForEachEntrySafe(ioctx, ioctx1, &read_head, element) {
                ioctx->trans_num = 0;
                listDel(&ioctx->element);
                ioctx->rc = false;
                socketReadDoneCallback(ioctx);
        }
        socketIODone(priv_p, conn_p, "socketDoRead2");   // subtract tp add ref
        return;
}

static ssize_t socketNoneBlockWrite(int sfd, void *buf, size_t len) {
        ssize_t nwrite = 0, n, to_write = len;
        char *p = buf;

        while (to_write) {
                n = write(sfd, p, to_write);
                if (n == 0) {
                	ELOG("write error 1: write 0 byte, remote close");
                        nwrite = -1; // the other side closed, return -1 to let the caller know the error
                        break;
                }
                if (n == -1) {
                        if (errno == EINTR)
                                continue; // interrupted, read again
                        else if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break; // EAGAIN/EWOULDBLOCK, return >=0 to let the caller treat it as success
                        ELOG("write error 2: write errno:%d", errno);
                        nwrite = -1;
                        break;
                }
                nwrite += n;
                to_write -= n;
                p += n;
        }
        return nwrite;
}

static void socketDoWrite(Job *job) {
        ConnectionLinux *conn_p = containerOf(job, ConnectionLinux, write_job);
        Socket *socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;
        IOContext *ioctx, *ioctx1;
        ListHead write_head;

        listHeadInit(&write_head);
        __sync_sub_and_fetch(&conn_p->is_writing, 1);
        pthread_spin_lock(&conn_p->write_jobs_head_lck);
        listJoinTailInit(&conn_p->write_jobs_head, &write_head);
        pthread_spin_unlock(&conn_p->write_jobs_head_lck);

        if (conn_p->is_closed) {
                goto closed;
        }

        listForEachEntrySafe(ioctx, ioctx1, &write_head, element) {
                ssize_t nwrite = socketNoneBlockWrite(conn_p->fd, ioctx->buf, ioctx->buf_size);
                if (nwrite == -1) {
                	//conn_p->super.m->close(&conn_p->super);
                        goto closed;
                } else if (nwrite == 0) {
                        break;
                } else {
                        ioctx->trans_num += nwrite;
                        if (nwrite < ioctx->buf_size) {
                                ioctx->buf += nwrite;
                                ioctx->buf_size -= nwrite;
                                break;
                        }
                        listDel(&ioctx->element);
                        ioctx->rc = true;
                        socketWriteDoneCallback(ioctx);
                }
        }
        if (!listEmpty(&write_head)) {
                pthread_spin_lock(&conn_p->write_jobs_head_lck);
                listJoinInit(&write_head, &conn_p->write_jobs_head);
                pthread_spin_unlock(&conn_p->write_jobs_head_lck);
        }
        socketIODone(priv_p, conn_p, "socketDoWrite1");   // subtract tp add ref
        return;
closed:
        listForEachEntrySafe(ioctx, ioctx1, &write_head, element) {
                ioctx->trans_num = 0;
                listDel(&ioctx->element);
                ioctx->rc = false;
                socketWriteDoneCallback(ioctx);
        }
        socketIODone(priv_p, conn_p, "socketDoWrite2");   // subtract tp add ref
        return;
}

static ConnectionLinux *createConnection(Socket *socket_p, int fd) {
        SocketPrivate *priv_p = socket_p->p;

        ConnectionLinux *conn_p = malloc(sizeof(*conn_p));
        conn_p->is_closed = 0;
        conn_p->is_reading = 0;
        conn_p->is_writing = 0;
        conn_p->ref = 1;
        conn_p->fd = fd;
        conn_p->read_job.doJob = socketDoRead;
        conn_p->write_job.doJob = socketDoWrite;
        listHeadInit(&conn_p->read_jobs_head);
        listHeadInit(&conn_p->write_jobs_head);
        pthread_spin_init(&conn_p->read_jobs_head_lck, 0);
        pthread_spin_init(&conn_p->write_jobs_head_lck, 0);
        conn_p->socket = socket_p;

        ConnectionLinuxParam param;
        param.fd = fd;
        bool rc = initConnectionLinux(&conn_p->super, &param);
        assert(rc);
        pthread_spin_lock(&priv_p->conn_lock);
        listAdd(&conn_p->element, &priv_p->conn_head);
        pthread_spin_unlock(&priv_p->conn_lock);
        return conn_p;
}

bool socketInsertReadJob(ConnectionLinux *conn_p) {
        Socket* socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;

        int ref = __sync_add_and_fetch(&conn_p->ref, 1);
        (void)ref;
        DLOG_SOCKET("socketInsertReadJob: ref:%d (%p)", ref, conn_p);
        return priv_p->param.read_tp->m->insertTailHash(priv_p->param.read_tp, &conn_p->read_job, conn_p->fd);
}

bool socketInsertWriteJob(ConnectionLinux *conn_p) {
        Socket* socket_p = conn_p->socket;
        SocketPrivate *priv_p = socket_p->p;

        int ref = __sync_add_and_fetch(&conn_p->ref, 1);
        (void)ref;
        DLOG_SOCKET("socketInsertWriteJob: ref:%d (%p)", ref, conn_p);
        return priv_p->param.write_tp->m->insertTailHash(priv_p->param.write_tp, &conn_p->write_job, conn_p->fd);
}

static void* socketCloseConnThread(void *p) {
        ConnectionLinux *conn_p = p;
        socketCloseConnection(conn_p);
        return NULL;
}

static void * socketEpollWaitingThread(void *p) {
        prctl(PR_SET_NAME, "socketEpollWaitingThread");
        Socket* socket_p = p;
        SocketPrivate *priv_p = socket_p->p;
        struct epoll_event event;
        struct epoll_event* events;
        ConnectionLinux *conn_p;
        int rc;
        bool rc1;

        priv_p->efd = epoll_create1(0);
        if (priv_p->efd == -1) {
                ELOG("epoll_create/() failed, error: %s", strerror(errno));
                assert(0);
        }
        memset(&event, 0, sizeof(event));
        event.data.fd = priv_p->sfd;
        event.events= EPOLLIN | EPOLLET;
        rc = epoll_ctl(priv_p->efd, EPOLL_CTL_ADD, priv_p->sfd, &event);
        if (rc == -1) {
                perror("epoll_ctl");
                assert(0);
        }

        /* Buffer where events are returned */
        events = calloc(MAXEVENTS, sizeof(event));

        /* The event loop */
        while (!priv_p->stop) {
                int n, i;
                n = epoll_wait(priv_p->efd, events, MAXEVENTS,10000); // 10 seconds
                for (i = 0; i < n; i++) {
                        if ((events[i].events & EPOLLERR)||
                                        (events[i].events & EPOLLHUP)||
                                        (!(events[i].events & (EPOLLIN | EPOLLOUT)))) {
                                /* An error has occured on this fd, or the socket is not
                                   ready for reading (why were we notified then?) */
                                if (events[i].data.fd == priv_p->sfd) {
                                        epoll_ctl(priv_p->efd, EPOLL_CTL_DEL,  events[i].data.fd, &event);
                                        continue;
                                }
                                conn_p = events[i].data.ptr;
                                ELOG("Server:Epoll error, tid:%lu, fd:%d, i:%d, event:%x (%p)\n", pthread_self(), events[i].data.fd, i, events[i].events, conn_p);
                                epoll_ctl(priv_p->efd, EPOLL_CTL_DEL,  events[i].data.fd, &event);

                                pthread_t tid;
                                pthread_attr_t attr;
                                pthread_attr_init(&attr);
                                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
                                pthread_create(&tid, &attr, socketCloseConnThread, conn_p);
                                continue;
                        } else if (priv_p->sfd == events[i].data.fd) {
                                /* We have a notification on the listening socket, which
                                   means one or more incoming connections. */
                                while (1) {
                                        struct sockaddr in_addr;
                                        socklen_t in_len;
                                        int infd;
                                        char hbuf[NI_MAXHOST],sbuf[NI_MAXSERV];

                                        in_len = sizeof in_addr;
                                        infd = accept(priv_p->sfd, &in_addr, &in_len);
                                        if (infd == -1) {
                                                if((errno== EAGAIN)|| (errno== EWOULDBLOCK)) {
                                                        /* We have processed all incoming
                                                           connections. */
                                                        break;
                                                } else {
                                                        ELOG("accept error");
                                                        break;
                                                }
                                        }

                                        rc = getnameinfo(&in_addr,in_len,
                                                        hbuf,sizeof hbuf,
                                                        sbuf,sizeof sbuf,
                                                        NI_NUMERICHOST | NI_NUMERICSERV);
                                        if (rc ==0) {
                                                DLOG_SOCKET("Accepted connection on descriptor %d "
                                                                "(host=%s, port=%s)\n", infd, hbuf, sbuf);
                                        }

                                        /* Make the incoming socket non-blocking and add it to the
                                           list of fds to monitor. */
                                        rc = makeSocketNonBlocking(infd);
                                        if(rc == -1)
                                                assert(0);

                                        conn_p = createConnection(socket_p, infd);
                                        event.data.fd = infd;
                                        event.data.ptr = conn_p;
                                        event.events= EPOLLIN | EPOLLOUT | EPOLLET;
                                        rc = epoll_ctl(priv_p->efd, EPOLL_CTL_ADD, infd, &event);
                                        if (rc == -1) {
                                                ELOG("epoll_ctl error");
                                                assert(0);
                                        }
                                        DLOG_SOCKET("SERVER: ON CONNECT(%p)", conn_p);
                                        priv_p->param.super.onConnect(&conn_p->super);
                                }
                                continue;
                        } else if (events[i].events & (EPOLLIN | EPOLLOUT)) {
                                conn_p = events[i].data.ptr;
                                if (events[i].events & EPOLLOUT) {
                                        DLOG_SOCKET("SERVER::EPOLL OUT(%p)", conn_p);
                                        if (!conn_p->is_writing) {
                                                int is_writing = __sync_add_and_fetch(&conn_p->is_writing, 1);
                                                if (is_writing == 1) {
                                                        DLOG_SOCKET("SERVER::EPOLL OUT INSERT WRITE(%p)", conn_p);
                                                        rc1 = socketInsertWriteJob(conn_p);
                                                        assert(rc1);
                                                } else {
                                                        __sync_sub_and_fetch(&conn_p->is_writing, 1);
                                                }
                                        }
                                }

                                if (events[i].events & EPOLLIN) {
                                        DLOG_SOCKET("SERVER::EPOLL IN(%p)", conn_p);
                                        if (!conn_p->is_reading) {
                                                int is_reading = __sync_add_and_fetch(&conn_p->is_reading, 1);
                                                if (is_reading == 1) {
                                                        DLOG_SOCKET("SERVER::EPOLL IN INSERT READ(%p)", conn_p);
                                                        rc1 = socketInsertReadJob(conn_p);
                                                        assert(rc1);
                                                } else {
                                                        __sync_sub_and_fetch(&conn_p->is_reading, 1);
                                                }
                                        }
                                }
                        } else {
                                assert(0);
                        }
                }
        }
        free(events);
        close(priv_p->efd);
        return NULL;
}

static int sockCreateAndBind(SocketPrivate *priv_p) {
        char port[16];
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int s,sfd;

        memset(&hints,0,sizeof(struct addrinfo));
        hints.ai_family= AF_UNSPEC;/* Return IPv4 and IPv6 */
        hints.ai_socktype= SOCK_STREAM;/* TCP socket */
        hints.ai_flags= AI_PASSIVE;/* All interfaces */
        sprintf(port, "%d", priv_p->param.super.port);

        s = getaddrinfo(NULL, port, &hints, &result); //more info about getaddrinfo() please see:man getaddrinfo!
        if(s != 0) {
                fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(s));
                return -1;
        }
        for(rp= result;rp!= NULL;rp=rp->ai_next) {
                sfd = socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
                if (sfd == -1)
                        continue;
                s =bind(sfd,rp->ai_addr,rp->ai_addrlen);
                if (s == 0) {
                        /* We managed to bind successfully! */
                        break;
                }
                close(sfd);
        }

        if (rp == NULL) {
                fprintf(stderr,"Could not bind\n");
                freeaddrinfo(result);
                return -1;
        }
        freeaddrinfo(result);
        return sfd;

}

static int sockPrepareServer(SocketPrivate *priv_p) {
        int rc;
        int sfd = -1;

        DLOG_SOCKET("Create socket on: %s:%d", priv_p->param.super.host, priv_p->param.super.port);
        sfd = sockCreateAndBind(priv_p);
        if (sfd == -1) {
                ELOG("Failed to create and bind the socket!\n");
                goto out;
        }

        rc = makeSocketNonBlocking(sfd);
        if (rc ==-1) assert(0);

        rc = listen(sfd, SOMAXCONN);
        if (rc != 0) {
                close(sfd);
                sfd = -1;
                ELOG("Failed to listen to socket %d, errno: %d!\n", sfd, errno);
                goto out;
        }

out:
        return sfd;
}

static bool socketServerStart(Socket *socket) {
        SocketPrivate *priv_p = socket->p;

        priv_p->sfd = sockPrepareServer(priv_p);
        if (priv_p->sfd == -1) return false;

        int rc = pthread_create(&priv_p->epoll_tid, NULL, socketEpollWaitingThread, (void *)socket);
        assert(rc == 0);
        return true;
}

/************CLIENT**************/

static inline in_addr_t hostToInetaddr(char *name) {
    struct hostent host_buf, *host = NULL;
    in_addr_t s_addr = 0;
    char tmp_buf[8192];
    int rc, host_err = 0;

    assert(name && name[0]);
    if ((rc = gethostbyname_r(name,
                    &host_buf, tmp_buf, sizeof(tmp_buf), &host, &host_err)) != 0
                    || host == NULL) {
        ELOG("gethostbyname_r name:%s, rc:%d, error:%s, host:%p",
                        name, rc, hstrerror(host_err), host);
        return s_addr;
    }

    const char *ip = inet_ntoa(*((struct in_addr *)host->h_addr_list[0]));
    DLOG_SOCKET("name:%s, ip:%s", name, ip);
    s_addr = inet_addr(ip);
    return s_addr;
}

static int sockPrepareClient(SocketPrivate *priv_p) {
        int sfd = -1;
        struct sockaddr_in saddr;
        int opt = 1;
        socklen_t len = sizeof(opt);
        int old_flags, ret;

        DLOG_SOCKET("Connect socket on: %s:%d", priv_p->param.super.host, priv_p->param.super.port);
        memset(&saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = hostToInetaddr(priv_p->param.super.host);
        if (saddr.sin_addr.s_addr == 0) {
                ELOG("can not convert host name:%s to valid ip addr", priv_p->param.super.host);
                sfd = -1;
                goto out;
        }
        saddr.sin_port = htons(priv_p->param.super.port);

        sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd < 0) {
                ELOG("failed to open a socket (%s, %d)",
                                priv_p->param.super.host, priv_p->param.super.port);
                sfd = -1;
                goto out;
        }

        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, len);
        if (setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, &opt, len) < 0) {
                ELOG("failed to do TCP_NODELAY, error: %s",
                                 strerror(errno));
                shutdown(sfd, SHUT_RDWR);
                close(sfd);
                sfd = -1;
                goto out;
        }

        old_flags = fcntl(sfd, F_GETFL, 0);
        if (old_flags < 0) {
                ELOG("failed to get flags of socket:%d to NONBLOCK, error: %s",
                        sfd, strerror(errno));
                shutdown(sfd, SHUT_RDWR);
                close(sfd);
                sfd = -1;
                goto out;
        }
        ret = fcntl(sfd, F_SETFL, old_flags | O_NONBLOCK);
        if (ret < 0) {
                ELOG("failed to set socket:%d to NONBLOCK, error: %s",
                        sfd, strerror(errno));
                shutdown(sfd, SHUT_RDWR);
                close(sfd);
                sfd = -1;
                goto out;
        }

        ret = connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr));
        if (!ret) {
                DLOG_SOCKET("socket:%d connected to %s:%d right away",
                                sfd, priv_p->param.super.host, priv_p->param.super.port);
                goto out;
        } else if (errno != EINPROGRESS) {
                ELOG("socket:%d failed to connect to %s:%d, error: %s",
                                sfd, priv_p->param.super.host, priv_p->param.super.port, strerror(errno));
                shutdown(sfd, SHUT_RDWR);
                close(sfd);
                sfd = -1;
                goto out;
        }
out:
        return sfd;
}

static void * socketEpollWaitingClientThread(void *p) {
        prctl(PR_SET_NAME, "socketEpollWaitingClientThread");
        Socket* socket_p = p;
        SocketPrivate *priv_p = socket_p->p;
        struct epoll_event event;
        struct epoll_event* events;
        ConnectionLinux *conn_p;
        int rc;
        bool rc1;

        priv_p->efd = epoll_create1(0);
        if (priv_p->efd == -1) {
                ELOG("epoll_create/() failed, error: %s", strerror(errno));
                assert(0);
        }
        conn_p = createConnection(socket_p, priv_p->cfd);
        event.data.fd = priv_p->cfd;
        event.data.ptr = conn_p;
        event.events= EPOLLIN | EPOLLOUT | EPOLLET;
        rc = epoll_ctl(priv_p->efd, EPOLL_CTL_ADD, priv_p->cfd, &event);
        if (rc == -1) {
                ELOG("epoll_ctl error");
                assert(0);
        }
        DLOG_SOCKET("Client:new connect(%p)", conn_p);
        priv_p->param.super.onConnect(&conn_p->super);

        /* Buffer where events are returned */
        events = calloc(1, sizeof(event));

        /* The event loop */
        while (!priv_p->stop) {
                int n, i;
                n = epoll_wait(priv_p->efd, events, 1, 1000); // 10 seconds
                for (i = 0; i < n; i++) {
                        if ((events[i].events & EPOLLERR)||
                                        (events[i].events & EPOLLHUP)||
                                        (!(events[i].events & (EPOLLIN | EPOLLOUT)))) {
                                /* An error has occured on this fd, or the socket is not
                                   ready for reading (why were we notified then?) */
                                conn_p = events[i].data.ptr;
                                ELOG("CLIENT: Epoll error, event:%x (%p)", events[i].events, conn_p);
                                epoll_ctl(priv_p->efd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                                socketCloseConnection(conn_p);
                                continue;
                        } else if (events[i].events & (EPOLLIN | EPOLLOUT)) {
                                conn_p = events[i].data.ptr;
                                if (events[i].events & EPOLLIN) {
                                        DLOG_SOCKET("CLIENT::EPOLL IN(%p)", conn_p);
                                        if (!conn_p->is_reading) {
                                                int is_reading = __sync_add_and_fetch(&conn_p->is_reading, 1);
                                                if (is_reading == 1) {
                                                        DLOG_SOCKET("CLIENT::EPOLL IN INSERT READ(%p)", conn_p);
                                                        rc1 = socketInsertReadJob(conn_p);
                                                        assert(rc1);
                                                } else {
                                                        __sync_sub_and_fetch(&conn_p->is_reading, 1);
                                                }
                                        }
                                }
                                if (events[i].events & EPOLLOUT) {
                                        DLOG_SOCKET("CLIENT::EPOLL OUT(%p)", conn_p);
                                        if (!conn_p->is_writing) {
                                                int is_writing = __sync_add_and_fetch(&conn_p->is_writing, 1);
                                                if (is_writing == 1) {
                                                        DLOG_SOCKET("CLIENT::EPOLL OUT INSERT WRITE(%p)", conn_p);
                                                        rc1 = socketInsertWriteJob(conn_p);
                                                        assert(rc1);
                                                } else {
                                                        __sync_sub_and_fetch(&conn_p->is_writing, 1);
                                                }
                                        }
                                }
                        } else {
                                assert(0);
                        }
                }
        }
        free(events);
        close(priv_p->efd);
        return NULL;
}

static bool socketClientStart(Socket *socket) {
        SocketPrivate *priv_p = socket->p;

        priv_p->cfd = sockPrepareClient(priv_p);
        if (priv_p->cfd == -1) return false;

        int rc = pthread_create(&priv_p->epoll_tid, NULL, socketEpollWaitingClientThread, (void *)socket);
        assert(rc == 0);
        return true;
}

static void* getContext(Socket *socket) {
        SocketPrivate *priv_p = socket->p;
        return priv_p->param.super.context;
}

static void destroy(Socket* obj) {
	SocketPrivate *priv_p = obj->p;
	ConnectionLinux *conn_p, *conn_p1;

	if (priv_p->sfd != -1) {
                shutdown(priv_p->sfd, SHUT_RDWR);
                close(priv_p->sfd);
	}
        pthread_spin_lock(&priv_p->conn_lock);
        listForEachEntrySafe(conn_p, conn_p1, &priv_p->conn_head, element) {
                conn_p->super.m->close(&conn_p->super);
//                shutdown(conn_p->fd, SHUT_RDWR);
        }
        while (!listEmpty(&priv_p->conn_head)) {
                pthread_spin_unlock(&priv_p->conn_lock);
                usleep(1000);
                pthread_spin_lock(&priv_p->conn_lock);
        }
        pthread_spin_unlock(&priv_p->conn_lock);
        priv_p->stop = true;
        pthread_join(priv_p->epoll_tid, NULL);

	free(priv_p);
}

static SocketMethod method = {
        .getContext = getContext,
        .destroy = destroy,
};

static void initSimphra() {
        struct sigaction sa;

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = SIG_IGN;
        if (sigaction(SIGTERM, &sa, NULL) == -1) {
                exit(-2);
        }
        sa.sa_handler = SIG_IGN;
        if (sigaction(SIGPIPE, &sa, NULL) == -1) {
                exit(-2);
        }
}

bool initSocketLinux(Socket* socket_p, SocketLinuxParam* param) {
	SocketPrivate *priv_p = malloc(sizeof(*priv_p));
	bool rc = true;

	initSimphra();
	socket_p->p = priv_p;
	socket_p->m = &method;
	memcpy(&priv_p->param, param, sizeof(*param));
	priv_p->sfd = -1;
	priv_p->cfd = -1;
	priv_p->stop = false;
	priv_p->ip_type = SOCKET_IP_TYPE_IPV4;
	priv_p->efd = -1;
        listHeadInit(&priv_p->conn_head);
        pthread_spin_init(&priv_p->conn_lock, 0);
        switch(param->super.type) {
        case SOCKET_TYPE_SERVER:
            rc = socketServerStart(socket_p);
            break;
        case SOCKET_TYPE_CLIENT:
            rc = socketClientStart(socket_p);
            break;
        default:
            assert(0);
        }
        if (rc == false) {
            free(priv_p);
        }
	return rc;
}

#endif




