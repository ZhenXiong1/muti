/*
 * Socket.c
 *
 *  Created on: 2020\12\9
 *      Author: Rick
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "Log.h"
#include "Object.h"
#include "network/Connection.h"
#include "network/Socket.h"
#include "ConnectionWindows.h"


#if defined(WIN32) || defined(WIN64)

typedef struct SocketPrivate {
        SocketParam             param;
        pthread_spinlock_t      conn_lock;
        ListHead                conn_head;
        pthread_t               *worker_tid;
        SocketIPType            ip_type;
        SOCKET                  sfd;
        HANDLE                  handle_completion;
        pthread_t               listen_tid;
        bool                    listen_stop;
        bool                    worker_stop;
} SocketPrivate;

static SOCKET sockCreateAndBind(SocketPrivate *priv_p)
{
        SocketIPType iptype = priv_p->ip_type;
        SOCKET sockfd = INVALID_SOCKET;
        int myport = priv_p->param.port;

        if  (iptype == SOCKET_IP_TYPE_IPV4) {
                struct sockaddr_in myaddr;

                if ((sockfd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED))
                                == INVALID_SOCKET) {
                        return INVALID_SOCKET;
                }

                ZeroMemory(&myaddr, sizeof(myaddr));
                myaddr.sin_family = PF_INET;
                myaddr.sin_port = htons(myport);
                myaddr.sin_addr.s_addr = INADDR_ANY;

                if (bind(sockfd, (struct sockaddr *) &myaddr, sizeof(struct sockaddr))
                        != 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }
        } else if (iptype == SOCKET_IP_TYPE_IPV6) {
                struct sockaddr_in6 myAddr;
                if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET) {
                        return INVALID_SOCKET;
                }

                ZeroMemory(&myAddr, sizeof(myAddr));
                myAddr.sin6_family = AF_INET6;
                myAddr.sin6_port = htons(myport);
                myAddr.sin6_addr = in6addr_any;

                if (bind(sockfd, (struct sockaddr *) &myAddr, sizeof(struct sockaddr_in6))
                        != 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }
        }

        return sockfd;
}

static int sockSetKeepAlive(SOCKET sockfd){
    int optval = 1;
    int optlen = sizeof(int);
    if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval, optlen) != 0){
        return -1;
    }
    return 0;
}

static SOCKET sockPrepareServer(SocketPrivate *priv_p)
{
        int s;
        SOCKET sfd = INVALID_SOCKET;

        DLOG("Create socket on: %s:%d", priv_p->param.host, priv_p->param.port);
        sfd = sockCreateAndBind(priv_p);
        if (sfd == INVALID_SOCKET) {
                ELOG("Failed to create and bind the socket!\n");
                goto out;
        }

        s = sockSetKeepAlive(sfd);
        if (s == -1) {
                close(sfd);
                sfd = INVALID_SOCKET;
                ELOG("Failed to set the socket %d keepalive options!\n", sfd);
                goto out;
        }

        s = listen(sfd, SOMAXCONN);
        if (s != 0) {
                close(sfd);
                sfd = INVALID_SOCKET;
                ELOG("Failed to listen to socket %d, errno: %d!\n",
                                sfd, errno);
                goto out;
        }

out:
        return sfd;
}

static bool socketAddNewSocketToIocp(Socket *socket_p, SOCKET new_sock) {
        SocketPrivate *priv_p = socket_p->p;
        ConnectionWin *conn_p = malloc(sizeof(*conn_p));
        ConnectionParam param;

        conn_p->ref = 1;
        conn_p->is_closed = 0;
        conn_p->socket = socket_p;
        param.fd = new_sock;
        bool rc = initConnectionWindows(&conn_p->super, &param);
        if (rc == false) return rc;
        if (CreateIoCompletionPort((HANDLE)new_sock, priv_p->handle_completion, 0, 0) == NULL) {
                ELOG("Binding Socket to IO Completion Port Failed::Reason Code::%ld", GetLastError());
                ((ConnectionWindowsMethod*)conn_p->super.m)->destroy(&conn_p->super);
                free(conn_p);
                return false;
        }
        pthread_spin_lock(&priv_p->conn_lock);
        listAdd(&conn_p->element, &priv_p->conn_head);
        pthread_spin_unlock(&priv_p->conn_lock);
        priv_p->param.onConnect(&conn_p->super);
        return true;
}

static void* socketListenThread(void* p) {
        Socket *socket_p = (Socket *)p;
        SocketPrivate *priv_p = socket_p->p;
        SOCKET sfd = priv_p->sfd;

        while (!priv_p->listen_stop) {
                struct sockaddr addr;
                int addrlen = sizeof(addr);
                SOCKET acceptSocket = accept(sfd, &addr, &addrlen);
                if (priv_p->listen_stop) break;
                if(acceptSocket == SOCKET_ERROR) {
                        ELOG("Error accept socket: %ld", GetLastError());
                        continue;
                }
                DLOG("new socket comes in:%d", acceptSocket);
                socketAddNewSocketToIocp(socket_p, acceptSocket);
        }
        return NULL;
}

static inline void socketIODone(SocketPrivate *priv_p, Connection *conn_p, int where) {
        ConnectionWin *connw_p = (ConnectionWin *)conn_p;
        int left = __sync_sub_and_fetch(&connw_p->ref, 1);
        if (left == 0) {
                DLOG("Release connection:%p", conn_p);
                pthread_spin_lock(&priv_p->conn_lock);
                listDel(&connw_p->element);
                pthread_spin_unlock(&priv_p->conn_lock);
                ((ConnectionWindowsMethod*)conn_p->m)->destroy(conn_p);
                free(conn_p);
        }
}

static inline void closeConnection(SocketPrivate *priv_p, Connection *conn_p) {
        ConnectionWin *connw_p = (ConnectionWin *)conn_p;
        int close = __sync_add_and_fetch(&connw_p->is_closed, 1);
        if (close == 1) {
                priv_p->param.onClose(conn_p);
                socketIODone(priv_p, conn_p, 1);
        }
}

static inline void closeConnectionNoLock(SocketPrivate *priv_p, Connection *conn_p) {
        ConnectionWin *connw_p = (ConnectionWin *)conn_p;
        int close = __sync_add_and_fetch(&connw_p->is_closed, 1);
        if (close == 1) {
                priv_p->param.onClose(conn_p);
                ((ConnectionWindowsMethod*)conn_p->m)->close(conn_p);
                int left = __sync_sub_and_fetch(&connw_p->ref, 1);
                if (left == 0) {
                        listDel(&connw_p->element);
                        ((ConnectionWindowsMethod*)conn_p->m)->destroy(conn_p);
                        free(conn_p);
                }
        }
}

#define IOCP_WAIT_TIMOUT 5000

static void* socketWorkerThread(void* p) {
        SocketPrivate *priv_p = (SocketPrivate *)p;
        HANDLE hCompletion = priv_p->handle_completion;
        void * lpCompletionKey = NULL;
        IOContext *ioctx = NULL;
        DWORD ioNum;
        DWORD err;

        while(!priv_p->worker_stop) {
                BOOL bOK = GetQueuedCompletionStatus(hCompletion,
                                &ioNum, (PULONG_PTR)&lpCompletionKey, (LPOVERLAPPED*)&ioctx, IOCP_WAIT_TIMOUT);
                if(!bOK)
                {
                        err = GetLastError();
                        if (err == WAIT_TIMEOUT) continue;
                        ELOG("Network error: %ld", err);
                        if (ioctx->type != IO_OPERATE_TYPE_STOP) {
                                if (ioctx->type == IO_OPERATE_TYPE_READ) {
                                        ioctx->readCallback(ioctx->conn, false, ioctx->wsabuf.buf, 0, ioctx->cb_arg);
                                } else {
                                        ioctx->writeCallback(ioctx->conn, false, ioctx->cb_arg);
                                }
                                closeConnection(priv_p, ioctx->conn);
                                socketIODone(priv_p, ioctx->conn, 2);
                        }
                        free(ioctx);
                        continue;
                }
                ioctx->transNum = ioNum;
                switch (ioctx->type) {
                case IO_OPERATE_TYPE_READ:
                        if (ioNum == 0) {
                                DLOG("Network Close");
                                ioctx->readCallback(ioctx->conn, false, ioctx->wsabuf.buf, 0, ioctx->cb_arg);
                                closeConnection(priv_p, ioctx->conn);
                                socketIODone(priv_p, ioctx->conn, 3);
                                free(ioctx);
                                continue;
                        }
                        ioctx->readCallback(ioctx->conn, true, ioctx->wsabuf.buf, ioNum, ioctx->cb_arg);
                        socketIODone(priv_p, ioctx->conn, 4);
                        break;
                case IO_OPERATE_TYPE_WRITE:
                        if (ioNum == 0) {
                                DLOG("Network Close");
                                ioctx->writeCallback(ioctx->conn, false, ioctx->cb_arg);
                                closeConnection(priv_p, ioctx->conn);
                                socketIODone(priv_p, ioctx->conn, 3);
                                free(ioctx);
                                continue;
                        }
                        ioctx->writeCallback(ioctx->conn, true, ioctx->cb_arg);
                        socketIODone(priv_p, ioctx->conn, 4);
                        break;
                case IO_OPERATE_TYPE_STOP:
                        break;
                case IO_OPERATE_TYPE_CLOSE:
                        closeConnection(priv_p, ioctx->conn);
                        break;
                default:
                        ELOG("Error type:%d", ioctx->type);
                        assert(0);
                }
                free(ioctx);
        }
        return NULL;
}

static bool socketServerStart(Socket *socket) {
        SocketPrivate *priv_p = socket->p;
        int i;

        priv_p->sfd = sockPrepareServer(priv_p);
        if (priv_p->sfd == INVALID_SOCKET) return false;
        priv_p->handle_completion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

        int rc = pthread_create(&priv_p->listen_tid, NULL, socketListenThread, (void *)socket);
        assert(rc == 0);

        priv_p->worker_tid = malloc(sizeof(pthread_t) * priv_p->param.working_thread_num);
        for (i=0; i<priv_p->param.working_thread_num; i++) {
            rc = pthread_create(&priv_p->worker_tid[i], NULL, socketWorkerThread, (void *)priv_p);
            assert(rc == 0);
        }
        return true;
}

/******************CLIENT******************************/

static int inet_pton(int af, const char *src, void *dst)
{
        struct sockaddr_storage ss;
        int size = sizeof(ss);
        char src_copy[INET6_ADDRSTRLEN+1];

        ZeroMemory(&ss, sizeof(ss));
        /* stupid non-const API */
        strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
        src_copy[INET6_ADDRSTRLEN] = 0;

        if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
                switch(af) {
                case AF_INET:
                        *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                        return 0;
                case AF_INET6:
                        *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                        return 0;
                }
        }
        return -1;
}

static SOCKET sockCreateAndConnect(SocketPrivate *priv_p)
{
        SocketIPType iptype = priv_p->ip_type;
        SOCKET sockfd = INVALID_SOCKET;
        int destport = priv_p->param.port;
        char *destip = priv_p->param.host;

        if (iptype == SOCKET_IP_TYPE_IPV4) {
                struct sockaddr_in dest;
                if ((sockfd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED))
                                == INVALID_SOCKET) {
                        return INVALID_SOCKET;
                }

                ZeroMemory(&dest, sizeof(dest));
                dest.sin_family = AF_INET;
                dest.sin_port = htons(destport);
                if (inet_pton(AF_INET, destip, &dest.sin_addr) < 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }

                if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }
        } else if (iptype == SOCKET_IP_TYPE_IPV6) {
                struct sockaddr_in6 dest;

                if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                        return INVALID_SOCKET;
                }

                ZeroMemory(&dest, sizeof(dest));
                dest.sin6_family = AF_INET6;
                dest.sin6_port = htons(destport);
                if (inet_pton(AF_INET6, destip, &dest.sin6_addr) < 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }

                if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
                        close(sockfd);
                        return INVALID_SOCKET;
                }
        }

        return sockfd;
}

#define SOCK_TIMEOUT 2000

static int sockSetTimeout(SOCKET sockFd, int ms){
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;

    if (setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) != 0) {
        return -1;
    }

    if (setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) != 0) {
        return -1;
    }

    return 0;
}

static SOCKET sockPrepareClient(SocketPrivate *priv_p)
{
        int s;
        SOCKET sfd = INVALID_SOCKET;

        sfd = sockCreateAndConnect(priv_p);
        if (sfd == INVALID_SOCKET) {
                ELOG("Failed to create and connect the socket!\n");
                goto out;
        }

        DLOG("Connected %s:%d on descriptor %d\n",
                        priv_p->param.host, priv_p->param.port, sfd);

        s = sockSetTimeout(sfd, SOCK_TIMEOUT);
        if (s == -1) {
                close(sfd);
                sfd = INVALID_SOCKET;
                ELOG("Failed to set the socket %d timeout option!\n", sfd);
                goto out;
        }

        s = sockSetKeepAlive(sfd);
        if (s == -1) {
                close(sfd);
                sfd = INVALID_SOCKET;
                ELOG("Failed to set the socket %d keepalive options!\n", sfd);
                goto out;
        }

out:
        return sfd;
}

static bool socketClientStart(Socket *socket_p) {
        SocketPrivate *priv_p = socket_p->p;
        int i, ret;

        assert(priv_p->param.type == SOCKET_TYPE_CLIENT);
        priv_p->sfd = sockPrepareClient(priv_p);
        if (priv_p->sfd == INVALID_SOCKET) return false;

        DLOG("Client Connect success!!\n");
        priv_p->handle_completion = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
        if (priv_p->handle_completion == NULL) {
                ELOG("CreateIoCompletionPort() Failed::Reason::%ld", GetLastError());
                closesocket(priv_p->sfd);
                return false;
        }
        socketAddNewSocketToIocp(socket_p, priv_p->sfd);
        priv_p->worker_tid = malloc(sizeof(pthread_t) * priv_p->param.working_thread_num);
        for (i=0; i<priv_p->param.working_thread_num; i++) {
            ret = pthread_create(&priv_p->worker_tid[i], NULL, socketWorkerThread, (void *)priv_p);
            assert(ret == 0);
        }
        return true;
}

static volatile int NumOfNetworkModule = 0;
static void socketModuleCleanup() {
    int err = WSACleanup();
    if (err != 0) {
        err =  WSAGetLastError();
        ELOG("WSAStartup failed with error: %d\n", err);
    }
}

void socketPostCloseEvent(Socket* obj, Connection *conn_p) {
        SocketPrivate *priv_p = obj->p;
        IOContext *ioctx;
        ioctx = calloc(1, sizeof(*ioctx));
        ioctx->type = IO_OPERATE_TYPE_CLOSE;
        ioctx->conn = conn_p;
        PostQueuedCompletionStatus(priv_p->handle_completion, 1024, (ULONG_PTR)NULL, &ioctx->overlap);
}

static void destroy(Socket* obj) {
        SocketPrivate *priv_p = obj->p;
        ConnectionWin *conn_p, *conn_p1;
        IOContext *ioctx;
        int i;

        ioctx = calloc(1, sizeof(*ioctx));
        ioctx->type = IO_OPERATE_TYPE_STOP;
        priv_p->listen_stop = true;
        PostQueuedCompletionStatus(priv_p->handle_completion, 1024, (ULONG_PTR)NULL, &ioctx->overlap);
        if (priv_p->param.type == SOCKET_TYPE_SERVER) {
                DLOG("Join Listener!");
                closesocket(priv_p->sfd);
                pthread_join(priv_p->listen_tid, NULL);
        }
        pthread_spin_lock(&priv_p->conn_lock);
        listForEachEntrySafe(conn_p, conn_p1, &priv_p->conn_head, element) {
                closeConnectionNoLock(priv_p, &conn_p->super);
        }
        pthread_spin_unlock(&priv_p->conn_lock);
        while (!listEmpty(&priv_p->conn_head)) {
                sleep(1);
        }
        priv_p->worker_stop = true;
        for (i=0; i<priv_p->param.working_thread_num; i++) {
                ioctx = calloc(1, sizeof(*ioctx));
                ioctx->type = IO_OPERATE_TYPE_STOP;
                PostQueuedCompletionStatus(priv_p->handle_completion, 1024, (ULONG_PTR)NULL, &ioctx->overlap);
                DLOG("Join worker thread:%d!", priv_p->param.type);
                pthread_join(priv_p->worker_tid[i], NULL);
        }

        free(priv_p->worker_tid);
        CloseHandle(priv_p->handle_completion);
        free(priv_p);

        if (!__sync_sub_and_fetch(&NumOfNetworkModule, 1)) {
            DLOG("Clean network module!");
            socketModuleCleanup();
        }
}

static SocketMethod method = {
        .destroy = destroy,
};

static void socketModuleLoad() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        ELOG("WSAStartup failed with error: %d\n", err);
        exit(1);
    }
}

bool initSocketWindows(Socket* socket_p, SocketParam* param) {
        if (!__sync_fetch_and_add(&NumOfNetworkModule, 1)) {
                socketModuleLoad();
        }

        SocketPrivate *priv_p = malloc(sizeof(*priv_p));
        int rc = true;

        socket_p->p = priv_p;
        socket_p->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));
        priv_p->ip_type = SOCKET_IP_TYPE_IPV4;
        listHeadInit(&priv_p->conn_head);
        pthread_spin_init(&priv_p->conn_lock, 0);
        priv_p->listen_stop = false;
        priv_p->worker_stop = false;

        switch(param->type) {
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

#else

typedef struct SocketPrivate {
        SocketParam     param;
} SocketPrivate;

static void destroy(Socket* obj) {
	SocketPrivate *priv_p = obj->p;
	free(priv_p);
}

static SocketMethod method = {
        .destroy = destroy,
};

bool initSocketWindows(Socket* socket_p, SocketParam* param) {
        SocketPrivate *priv_p = malloc(sizeof(*priv_p));
        int rc = true;

        socket_p->p = priv_p;
        socket_p->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));
        return rc;
}
#endif





