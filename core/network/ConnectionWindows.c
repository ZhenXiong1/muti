/*
 * ConnectionWindows.c
 *
 *  Created on: 2020\12\9
 *      Author: Rick
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Object.h"
#include "network/Connection.h"
#include "Log.h"
#include "ConnectionWindows.h"

#if defined(WIN32) || defined(WIN64)

extern void socketPostCloseEvent(Socket* obj, Connection *conn_p);

typedef struct ConnectionPrivate {
        ConnectionParam param;
        void            *ctx;
} ConnectionPrivate;

void connSetContext(Connection *conn_p, void *ctx) {
        ConnectionPrivate *priv_p = conn_p->p;
        priv_p->ctx = ctx;
}

void* connGetContext(Connection *conn_p) {
        ConnectionPrivate *priv_p = conn_p->p;
        return priv_p->ctx;
}

static bool connRead(Connection* conn_p, void* buf, size_t sz, ConnectionReadCallback cb, void *arg) {
        ConnectionPrivate *priv_p = conn_p->p;
        SOCKET s = priv_p->param.fd;
        ConnectionWin *conn_win_p = (ConnectionWin *)conn_p;
        IOContext *iocxt;
        DWORD flag = 0;

        if (conn_win_p->is_closed) return false;
        iocxt = malloc(sizeof(IOContext));
        iocxt->readCallback = cb;
        iocxt->cb_arg = arg;
        iocxt->type = IO_OPERATE_TYPE_READ;
        iocxt->wsabuf.buf = buf;
        iocxt->wsabuf.len = sz;
        iocxt->conn = conn_p;
        memset(&iocxt->overlap, 0, sizeof(OVERLAPPED));
        __sync_add_and_fetch(&conn_win_p->ref, 1);
        int ret = WSARecv((SOCKET)s, &iocxt->wsabuf, 1, &iocxt->transNum,
                &flag, &iocxt->overlap, NULL);
        if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())){
                int ref = __sync_sub_and_fetch(&conn_win_p->ref, 1);
                assert(ref != 0);
                ELOG("WASRecv Failed::Reason Code::%d,%d,%d", WSAGetLastError(), ref, conn_win_p->is_closed);
                socketPostCloseEvent(conn_win_p->socket, conn_p);
                free(iocxt);
                return false;
        }
        return true;
}

static bool connWrite(Connection* conn_p, void* buf, size_t sz, ConnectionWriteCallback cb, void *arg) {
        ConnectionPrivate *priv_p = conn_p->p;
        ConnectionWin *conn_win_p = (ConnectionWin *)conn_p;
        SOCKET s = priv_p->param.fd;
        IOContext *iocxt;

        if (conn_win_p->is_closed) return false;
        iocxt = malloc(sizeof(IOContext));
        iocxt->writeCallback = cb;
        iocxt->cb_arg = arg;
        iocxt->type = IO_OPERATE_TYPE_WRITE;
        iocxt->wsabuf.buf = buf;
        iocxt->wsabuf.len = sz;
        iocxt->conn = conn_p;
        memset(&iocxt->overlap, 0, sizeof(OVERLAPPED));

        __sync_add_and_fetch(&conn_win_p->ref, 1);
        int ret = WSASend((SOCKET)s, &iocxt->wsabuf, 1, &iocxt->transNum,
                0, (LPWSAOVERLAPPED)&iocxt->overlap, (LPWSAOVERLAPPED_COMPLETION_ROUTINE)NULL);
        if(ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())){
                int ref = __sync_sub_and_fetch(&conn_win_p->ref, 1);
                assert(ref != 0);
                ELOG("WASSend Failed::Reason Code::%d,%d,%d", WSAGetLastError(), ref, conn_win_p->is_closed);
                socketPostCloseEvent(conn_win_p->socket, conn_p);
                free(iocxt);
                return false;
        }
        return true;
}

static void destroy(Connection* conn_p) {
        ConnectionPrivate *priv_p = conn_p->p;
        if (priv_p->param.fd != INVALID_SOCKET) {
                closesocket(priv_p->param.fd);
        }
        free(priv_p);
}

static void connClose(Connection* conn_p) {
        ConnectionPrivate *priv_p = conn_p->p;
        if (priv_p->param.fd != INVALID_SOCKET) {
                closesocket(priv_p->param.fd);
                priv_p->param.fd = INVALID_SOCKET;
        }
}

static ConnectionWindowsMethod method = {
        .super = {
                 .read = connRead,
                 .write = connWrite,
                 .getContext = connGetContext,
                 .setContext = connSetContext
        },
        .destroy = destroy,
        .close = connClose,
};

bool initConnectionWindows(Connection* obj, ConnectionParam* param) {
	ConnectionPrivate *priv_p = malloc(sizeof(*priv_p));

	obj->p = priv_p;
	obj->m = &method.super;
	memcpy(&priv_p->param, param, sizeof(*param));

	return true;
}

#endif




