/*
 * ConnectionLinux.c
 *
 *  Created on: 2020\12\9
 *      Author: Rick
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "Object.h"
#include "network/Connection.h"
#include "Log.h"
#include "ConnectionLinux.h"

#if defined(WIN32) || defined(WIN64)
#else

#include <sys/socket.h>

//#define DLOG_CONN(str, args...) DLOG(str, ##args)
#define DLOG_CONN(str, args...)

typedef struct ConnectionPrivate {
        ConnectionLinuxParam    param;
        void                    *ctx;
} ConnectionPrivate;

static void connSetContext(Connection *conn_p, void *ctx) {
        ConnectionPrivate *priv_p = conn_p->p;
        priv_p->ctx = ctx;
}

static void* connGetContext(Connection *conn_p) {
        ConnectionPrivate *priv_p = conn_p->p;
        return priv_p->ctx;
}

static Socket* connGetSocket(Connection *conn_p) {
        ConnectionLinux *conn_lp = containerOf(conn_p, ConnectionLinux, super);
        return conn_lp->socket;
}

extern bool socketInsertReadJob(ConnectionLinux *conn_p);

static bool connRead(Connection* conn_p, void* buf, size_t sz, ConnectionReadCallback cb, void *arg) {
        ConnectionLinux *conn_lp = containerOf(conn_p, ConnectionLinux, super);
        bool rc;

        if (__sync_add_and_fetch(&conn_lp->is_closed, 0)) return false;
        IOContext *ctx = malloc(sizeof(*ctx));
        ctx->buf = buf;
        ctx->buf_size = sz;
        ctx->readCallback = cb;
        ctx->cb_arg = arg;
        ctx->conn = conn_p;
        ctx->rc = false;
        ctx->trans_num = 0;
        __sync_add_and_fetch(&conn_lp->ref, 1);
        pthread_spin_lock(&conn_lp->read_jobs_head_lck);
        listAddTail(&ctx->element, &conn_lp->read_jobs_head);
        pthread_spin_unlock(&conn_lp->read_jobs_head_lck);
        int is_reading = __sync_add_and_fetch(&conn_lp->is_reading, 1);
        if (is_reading == 1) {
                DLOG_CONN("CONN::Insert read job(%p)", conn_lp);
                rc = socketInsertReadJob(conn_lp);
                assert(rc == true);
        } else {
                __sync_sub_and_fetch(&conn_lp->is_reading, 1);
        }
        return true;
}

extern bool socketInsertWriteJob(ConnectionLinux *conn_p);

static bool connWrite(Connection* conn_p, void* buf, size_t sz, ConnectionWriteCallback cb, void *arg) {
        ConnectionLinux *conn_lp = containerOf(conn_p, ConnectionLinux, super);
        bool rc;

        if (__sync_add_and_fetch(&conn_lp->is_closed, 0)) return false;
        IOContext *ctx = malloc(sizeof(*ctx));
        ctx->buf = buf;
        ctx->buf_size = sz;
        ctx->writeCallback = cb;
        ctx->cb_arg = arg;
        ctx->conn = conn_p;
        ctx->rc = false;
        ctx->trans_num = 0;
        __sync_add_and_fetch(&conn_lp->ref, 1);
        pthread_spin_lock(&conn_lp->write_jobs_head_lck);
        listAddTail(&ctx->element, &conn_lp->write_jobs_head);
        pthread_spin_unlock(&conn_lp->write_jobs_head_lck);
        int is_writing = __sync_add_and_fetch(&conn_lp->is_writing, 1);
        if (is_writing == 1) {
                DLOG_CONN("CONN::Insert write job(%p)", conn_lp);
                rc = socketInsertWriteJob(conn_lp);
                assert(rc == true);
        } else {
                __sync_sub_and_fetch(&conn_lp->is_writing, 1);
        }
        return true;
}

//void socketCloseConnection(ConnectionLinux *conn_p);

static void connClose(Connection *conn_p) {
        ConnectionLinux *conn_lp = containerOf(conn_p, ConnectionLinux, super);
        shutdown(conn_lp->fd, SHUT_RDWR);
//        socketCloseConnection(conn_lp);
}

static void destroy(Connection* conn_p) {
        ConnectionPrivate *priv_p = conn_p->p;
        if (priv_p->param.fd != -1) {
                shutdown(priv_p->param.fd, SHUT_RDWR);
                close(priv_p->param.fd);
        }
        free(priv_p);
}


static ConnectionLinuxMethod method = {
        .super = {
                 .read = connRead,
                 .write = connWrite,
                 .close = connClose,
                 .getSocket = connGetSocket,
                 .getContext = connGetContext,
                 .setContext = connSetContext
        },
        .destroy = destroy,
};

bool initConnectionLinux(Connection* obj, ConnectionLinuxParam* param) {
	ConnectionPrivate *priv_p = malloc(sizeof(*priv_p));

	obj->p = priv_p;
	obj->m = &method.super;
	memcpy(&priv_p->param, param, sizeof(*param));

	return true;
}

#endif




