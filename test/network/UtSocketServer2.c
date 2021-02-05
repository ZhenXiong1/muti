/*
 * UtSocketServer.c
 *
 *  Created on: 2020\12\14
 *      Author: Rick
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

#include "network/Socket.h"
#include "Log.h"
#if defined(WIN32) || defined(WIN64)
#include <windows.h>
#define sleep(n) Sleep(n * 1000)
#endif

typedef struct UtSocketConnCtx {
        int     test;
} UtSocketConnCtx;

static void utReadCallback(Connection *conn, bool rc, void* buffer, size_t sz, void *cbarg) {
        (void)cbarg;
        if (rc == false) {
                ELOG("Server::Read failed!");
        } else {
                char *sbuf = buffer;
                sbuf[sz] = '\0';
                DLOG("Server::Read success:%s", sbuf);
        }
        free(buffer);
        conn->m->close(conn);
}

static void utWriteCallback(Connection *conn, bool rc, void *cbarg) {
        char *buffer = cbarg;
        if (rc == false) {
                ELOG("Server::Write failed!");
        } else {
                DLOG("Write success, wait response ...");
                rc = conn->m->read(conn, buffer, 1023, utReadCallback, cbarg);
                if (rc == false) {
                        free(cbarg);
                        assert(0);
                }
        }
}

static void utSocketServerOnClose(Connection *conn) {
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        free(ctx);
        DLOG("utSocketServerOnClose");
}

static void utSocketServerOnConnect(Connection *conn) {
        UtSocketConnCtx *ctx = calloc(1, sizeof(*ctx));
        conn->m->setContext(conn, ctx);
        char *buffer = malloc(1024);
        sprintf(buffer, "Hello, This is server.");
        bool rc = conn->m->write(conn, buffer, strlen(buffer), utWriteCallback, buffer);
        if (rc == false) {
                free(buffer);
                assert(0);
        }
}

int UtSocketServer2(int argv, char **argvs) {
        (void)argv; (void)argvs;
        bool rc;
        Socket socket;
#if defined(WIN32) || defined(WIN64)
        SocketParam param;
        strcpy(param.host, "127.0.0.1");
        param.onClose = utSocketServerOnClose;
        param.onConnect = utSocketServerOnConnect;
        param.port = 10186;
        param.type = SOCKET_TYPE_SERVER;
        param.working_thread_num = 6;
        rc = initSocketWindows(&socket, &param);
        assert(rc == true);
#else
        ThreadPool tp1, tp2;
        ThreadPoolParam param_tp;
        param_tp.do_batch = NULL;
        param_tp.thread_number = 16;
        rc = initThreadPool(&tp1, &param_tp);
        assert(rc);
        rc = initThreadPool(&tp2, &param_tp);
        assert(rc);

        SocketLinuxParam param;
        strcpy(param.super.host, "127.0.0.1");
        param.super.onClose = utSocketServerOnClose;
        param.super.onConnect = utSocketServerOnConnect;
        param.super.port = 10186;
        param.super.type = SOCKET_TYPE_SERVER;
        param.read_tp = &tp1;
        param.write_tp = &tp2;
        rc = initSocketLinux(&socket, &param);
        assert(rc == true);
#endif
        sleep(60);
        DLOG("STOP Server!!!!!");
        socket.m->destroy(&socket);
#if defined(WIN32) || defined(WIN64)
#else
        tp1.m->destroy(&tp1);
        tp2.m->destroy(&tp2);
#endif
	return 0;

}


