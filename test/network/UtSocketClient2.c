/*
 * UtSocketClient.c
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

static int UtSocketStop = 0;

static void utWriteCallback(Connection *conn, bool rc, void *cbarg) {

        if (rc == false) {
                ELOG("Write failed!");
                assert(0);
        } else {
                DLOG("Client write success\n");
        }
        free(cbarg);
        conn->m->close(conn);
        UtSocketStop = 1;
}

static void utReadCallback(Connection *conn, bool rc, void* buffer, size_t sz, void *cbarg) {
        UtSocketConnCtx *ctx = cbarg;
        if (rc == false) {
                ELOG("Client::Read failed!");
                conn->m->close(conn);
                UtSocketStop = 1;
                free(buffer);
                return;
        } else {
                char *sbuf = buffer;
                sbuf[sz] = '\0';
                DLOG("Client::Read success:%s", sbuf);
        }
        sprintf(buffer, "Hello, This is client.");
        rc = conn->m->write(conn, buffer, strlen(buffer), utWriteCallback, ctx);
        if (rc == false) {
                ELOG("Client::Write failed!");
                conn->m->close(conn);
                UtSocketStop = 1;
                free(buffer);
                return;
        }
}

static void utSocketClientOnClose(Connection *conn) {
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        free(ctx);
        DLOG("utSocketClientOnClose");
}

static void utSocketClientOnConnect(Connection *conn) {
        UtSocketConnCtx *ctx = calloc(1, sizeof(*ctx));
        conn->m->setContext(conn, ctx);
        char *buffer= malloc(1024);
        DLOG("Receiving from server ...");
        bool rc = conn->m->read(conn, buffer, 1023, utReadCallback, ctx);
        if (rc == false) {
                ELOG("Client::Read failed1!");
                conn->m->close(conn);
                UtSocketStop = 1;
                free(buffer);
        }
}

int UtSocketClient2(int argv, char **argvs) {
        (void)argv; (void)argvs;
        Socket socket;
        bool rc;

#if defined(WIN32) || defined(WIN64)
        SocketParam param;
        strcpy(param.host, "127.0.0.1");
        param.onClose = utSocketClientOnClose;
        param.onConnect = utSocketClientOnConnect;
        param.port = 10186;
        param.type = SOCKET_TYPE_CLIENT;
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
        param.super.onClose = utSocketClientOnClose;
        param.super.onConnect = utSocketClientOnConnect;
        param.super.port = 10186;
        param.super.type = SOCKET_TYPE_CLIENT;
        param.read_tp = &tp1;
        param.write_tp = &tp2;
        rc = initSocketLinux(&socket, &param);
        assert(rc == true);
#endif
        while (UtSocketStop == 0) {
                sleep(1);
        }
        DLOG("STOP Client!!!!!");
        socket.m->destroy(&socket);
#if defined(WIN32) || defined(WIN64)
#else
        tp1.m->destroy(&tp1);
        tp2.m->destroy(&tp2);
#endif

        return 0;
}
