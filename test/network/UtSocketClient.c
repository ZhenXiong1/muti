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

#define UT_BUFFER_SIZE 4096

typedef struct UtSocketConnCtx {
        pthread_t               read_tid;
        pthread_t               write_tid;
        int                     stop;
        volatile uint64_t       read_req_num;
        volatile uint64_t       read_req_done_num;
        volatile uint64_t       write_req_num;
        volatile uint64_t       write_req_done_num;
        volatile uint64_t       read_success_req_num;
        volatile uint64_t       write_success_req_num;
} UtSocketConnCtx;

static void utReadCallback(Connection *conn, bool rc, void* buffer, size_t sz, void *cbarg) {
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        if (rc == false) {
                ELOG("Client::Read failed!");
        } else {
                char *sbuf = buffer;
                sbuf[sz] = '\0';
                DLOG("Client::Read success:%s", sbuf);
                __sync_add_and_fetch(&ctx->read_success_req_num, 1);
        }
        __sync_add_and_fetch(&ctx->read_req_done_num, 1);
        free(cbarg);
}

static void* utSocketReadThread(void *p) {
        Connection *conn = p;
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        while(!ctx->stop) {
                char *buffer= malloc(UT_BUFFER_SIZE);
                __sync_add_and_fetch(&ctx->read_req_num, 1);
                bool rc = conn->m->read(conn, buffer, UT_BUFFER_SIZE - 1, utReadCallback, buffer);
                if (rc == false) {
                        free(buffer);
                        __sync_add_and_fetch(&ctx->read_req_done_num, 1);
                }
                while(ctx->read_req_num - ctx->read_req_done_num > 1024) {
                        usleep(1000);
                }
        }
        while (ctx->read_req_num != ctx->read_req_done_num) {
                DLOG("ctx->read_req_num:%ld ctx->read_req_done_num:%ld", (long)ctx->read_req_num, (long)ctx->read_req_done_num);
                sleep(1);
        }
        return NULL;
}

static void utWriteCallback(Connection *conn, bool rc, void *cbarg) {
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        if (rc == false) {
                ELOG("Write failed!");
        } else {
                DLOG("Write success\n");
                __sync_add_and_fetch(&ctx->write_success_req_num, 1);
        }
        __sync_add_and_fetch(&ctx->write_req_done_num, 1);

        free(cbarg);
}

static void* utSocketWriteThread(void *p) {
        Connection *conn = p;
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        while(!ctx->stop) {
                char *buffer= malloc(UT_BUFFER_SIZE);
                uint64_t req_num = __sync_add_and_fetch(&ctx->write_req_num, 1);
                sprintf(buffer, "This is from client, number:%ld", (long)req_num);
                bool rc = conn->m->write(conn, buffer, strlen(buffer), utWriteCallback, buffer);
                if (rc == false) {
                        free(buffer);
                        __sync_add_and_fetch(&ctx->write_req_done_num, 1);
                }
                while(ctx->write_req_num - ctx->write_req_done_num > 1024) {
                        usleep(1000);
                }
        }
        while (ctx->write_req_num != ctx->write_req_done_num) {
                DLOG("ctx->write_req_num:%ld ctx->write_req_done_num:%ld", (long)ctx->write_req_num, (long)ctx->write_req_done_num);
                sleep(1);
        }
        return NULL;
}

static void utSocketClientOnClose(Connection *conn) {
        UtSocketConnCtx *ctx = conn->m->getContext(conn);
        ctx->stop = 1;
        pthread_join(ctx->read_tid, NULL);
        pthread_join(ctx->write_tid, NULL);
        DLOG("utSocketClientOnClose, Read:%ld Write:%ld", (long)ctx->read_success_req_num, (long)ctx->write_success_req_num);
        free(ctx);
}

static void utSocketClientOnConnect(Connection *conn) {
        UtSocketConnCtx *ctx = calloc(1, sizeof(*ctx));
        conn->m->setContext(conn, ctx);
        pthread_create(&ctx->read_tid, NULL, utSocketReadThread, conn);
        pthread_create(&ctx->write_tid, NULL, utSocketWriteThread, conn);
}

int UtSocketClient(int argv, char **argvs) {
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
        param_tp.thread_number = 1;
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
        sleep(30);
        DLOG("STOP!!!!!");
        socket.m->destroy(&socket);
#if defined(WIN32) || defined(WIN64)
#else
        tp1.m->destroy(&tp1);
        tp2.m->destroy(&tp2);
#endif

        return 0;
}
