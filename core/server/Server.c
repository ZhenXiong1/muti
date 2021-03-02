/*
 * Server.c
 *
 *  Created on: Jan 15, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include <server/Server.h>
#include <network/Socket.h>
#include <util/ThreadPool.h>
#include <Log.h>

typedef struct ServerPrivate {
        ServerParam     param;
        Socket          socket;
        ThreadPool      tp_read;
        ThreadPool      tp_write;
} ServerPrivate;

typedef struct SConnectionContext {
        volatile uint64_t       read_counter;
        volatile uint64_t       write_counter;
        volatile uint64_t       write_done_counter;
        volatile uint64_t       received_request_counter;
        volatile uint64_t       sent_request_counter;
        Readbuffer              *to_read_buf;
        pthread_mutex_t         to_read_buf_lock;
} SConnectionContext;

static void serverReadCallback(Connection* conn_p, bool rc, void* buffer, size_t sz, void *cbarg);

static Readbuffer* serverCreateReadBuffer(ServerPrivate *priv_p) {
        Readbuffer *rbuf = malloc(sizeof(*rbuf));

        rbuf->buffer = malloc(priv_p->param.read_buffer_size);
        rbuf->read_buffer_start = 0;
        rbuf->reference = 0;
        return rbuf;
}

static inline void serverReadDone(Connection *conn_p) {
        Socket* skt = conn_p->m->getSocket(conn_p);
        Server* srv = skt->m->getContext(skt);
        ServerPrivate *priv_p = srv->p;
        SConnectionContext *ccxt = conn_p->m->getContext(conn_p);

        uint64_t left = __sync_sub_and_fetch(&ccxt->read_counter, 1);
        if (left == priv_p->param.max_read_buffer_counter / 2) {
                pthread_mutex_lock(&ccxt->to_read_buf_lock);
                if (ccxt->to_read_buf){
                        Readbuffer *rbuf = ccxt->to_read_buf;
                        bool rc = conn_p->m->read(conn_p, rbuf->buffer + rbuf->read_buffer_start,
                                        priv_p->param.read_buffer_size - rbuf->read_buffer_start,
                                        serverReadCallback, rbuf);
                        if (rc == false) {
                                free(rbuf->buffer);
                                free(rbuf);
                                __sync_sub_and_fetch(&ccxt->read_counter, 1);
                        }
                        ccxt->to_read_buf = NULL;
                }
                pthread_mutex_unlock(&ccxt->to_read_buf_lock);
        }
}

static void serverWriteCallback(Connection* conn_p, bool rc, void *buffer) {
        SConnectionContext *ccxt = conn_p->m->getContext(conn_p);
        if (rc == false) {
                ELOG("error send buffer!!");
        }
        __sync_add_and_fetch(&ccxt->write_done_counter, 1);
        __sync_add_and_fetch(&ccxt->sent_request_counter, 1);
        free(buffer);
}

static void serverDoActionCallback(SRequest *reqw) {
        Connection* conn_p = reqw->connection;
        SConnectionContext *ccxt = conn_p->m->getContext(conn_p);
        char *buffer;
        size_t buff_len;
        bool free_resp;

        assert(reqw->response != NULL);
        int8_t error_id = reqw->response->error_id;
        if (error_id == 0) {
                RequestHandler *res = reqw->req_handler;
                res->response_encoders[reqw->request->request_id]
                         (reqw->response, &buffer, &buff_len, &free_resp);
        } else {
                ErrorResponseEncoder(reqw->response, &buffer, &buff_len, &free_resp);
        }
        __sync_add_and_fetch(&ccxt->write_counter, 1);
        bool rc = conn_p->m->write(conn_p, buffer, buff_len, serverWriteCallback, buffer);
        if (rc == false) {
                free(buffer);
                __sync_add_and_fetch(&ccxt->sent_request_counter, 1);
                __sync_add_and_fetch(&ccxt->write_done_counter, 1);
        }
        if (free_resp) {
                free(reqw->response);
        }
        if (reqw->free_req) {
                free(reqw->request);
        }
        uint32_t left = __sync_sub_and_fetch(&reqw->read_buffer->reference, 1);
        if (left == 0) {
                free(reqw->read_buffer->buffer);
                free(reqw->read_buffer);
                serverReadDone(conn_p);
        }
        free(reqw);
}

static void serverDoAction(Job *job) {
        SRequest *reqw = containerOf(job, SRequest, job);
        RequestHandler *handler = reqw->req_handler;

        handler->actions[reqw->request->request_id](reqw);
}

static inline void serverRead(Connection* conn_p, ServerPrivate *priv_p, char *buf, size_t buf_len) {
        SConnectionContext *ccxt = conn_p->m->getContext(conn_p);
        Readbuffer *rbuf = serverCreateReadBuffer(priv_p);
        memcpy(rbuf->buffer, buf, buf_len);
        rbuf->read_buffer_start = buf_len;

        uint64_t left = __sync_add_and_fetch(&ccxt->read_counter, 1);
        if (left == priv_p->param.max_read_buffer_counter) {
                pthread_mutex_lock(&ccxt->to_read_buf_lock);
                assert(ccxt->to_read_buf == NULL);
                ccxt->to_read_buf = rbuf;
                pthread_mutex_unlock(&ccxt->to_read_buf_lock);
                return;
        }
        bool rc = conn_p->m->read(conn_p, rbuf->buffer + rbuf->read_buffer_start,
                        priv_p->param.read_buffer_size - rbuf->read_buffer_start,
                        serverReadCallback, rbuf);
        if (rc == false) {
                free(rbuf->buffer);
                free(rbuf);
                __sync_sub_and_fetch(&ccxt->read_counter, 1);
        }
}

static void serverReadCallback(Connection* conn_p, bool rc, void* buffer, size_t sz, void *cbarg) {
        Socket* skt = conn_p->m->getSocket(conn_p);
        Server* srv = skt->m->getContext(skt);
        ServerPrivate *priv_p = srv->p;
        SConnectionContext *ccxt = conn_p->m->getContext(conn_p);
        Readbuffer *rbuf = cbarg;
        uint16_t resource_id;
        uint16_t request_id;
        char *buf;
        size_t buf_len;
        uint32_t left;

        if (rc == false) {
                free(rbuf->buffer);
                free(rbuf);
                __sync_sub_and_fetch(&ccxt->read_counter, 1);
                return;
        }

        buf = rbuf->buffer;
        buf_len = rbuf->read_buffer_start + sz;
        rbuf->reference = 1;

        while(true) {
                if (buf_len < 4) {
                        serverRead(conn_p, priv_p, buf, buf_len);
                        break;
                }
                Request *req1 = (Request*)buf;
                resource_id = req1->resource_id;
                if (resource_id > priv_p->param.request_handler_length) {
                        rc = false;
                        goto out;
                }
                RequestHandler *handler = &priv_p->param.request_handler[resource_id];
                if (handler == NULL) {
                        rc = false;
                        goto out;
                }
                RequestDecoder *req_decoders = handler->request_decoders;
                if (req_decoders == NULL) {
                        rc = false;
                        goto out;
                }
                request_id = req1->request_id; // TODO: High low bytes
                // TODO: Validate request id
                RequestDecoder reqDecoder = req_decoders[request_id];
                if (reqDecoder == NULL) {
                        rc = false;
                        goto out;
                }
                size_t consume_len;
                bool free_req;
                Request *req = reqDecoder(buf, buf_len, &consume_len, &free_req);
                if (req) {
                        SRequest *reqw = malloc(sizeof(*reqw));
                        reqw->connection = conn_p;
                        reqw->read_buffer = rbuf;
                        reqw->request = req;
                        reqw->req_handler = handler;
                        reqw->response = NULL;
                        reqw->free_req = free_req;
                        reqw->action_callback = serverDoActionCallback;
                        buf += consume_len;
                        buf_len -= consume_len;
                        __sync_add_and_fetch(&rbuf->reference, 1);
                        reqw->job.doJob = serverDoAction;
                        __sync_add_and_fetch(&ccxt->received_request_counter, 1);
                        rc = priv_p->param.worker_tp->m->insertTail(priv_p->param.worker_tp, &reqw->job);
                        if (rc == false) {
                                __sync_sub_and_fetch(&rbuf->reference, 1);
                                __sync_add_and_fetch(&ccxt->sent_request_counter, 1);
                                goto out;
                        }
                } else {
                        serverRead(conn_p, priv_p, buf, buf_len);
                        break;
                }
        }
out:
        left = __sync_sub_and_fetch(&rbuf->reference, 1);
        if (left == 0) {
                free(rbuf->buffer);
                free(rbuf);
                serverReadDone(conn_p);
        }
        if (rc == false) conn_p->m->close(conn_p);
}

static void serverOnConnect(Connection *conn) {
        Socket* skt = conn->m->getSocket(conn);
        Server* srv = skt->m->getContext(skt);
        ServerPrivate *priv_p = srv->p;
        SConnectionContext *ccxt = calloc(1, sizeof(SConnectionContext));
        pthread_mutex_init(&ccxt->to_read_buf_lock, NULL);
        Readbuffer *rbuf = serverCreateReadBuffer(priv_p);

        conn->m->setContext(conn, ccxt);
        ccxt->read_counter ++;
        conn->m->read(conn, rbuf->buffer,
                        priv_p->param.read_buffer_size,
                        serverReadCallback, rbuf);
}

static void serverOnClose(Connection *conn) {
        SConnectionContext *ccxt;
        ccxt = conn->m->getContext(conn);

        pthread_mutex_lock(&ccxt->to_read_buf_lock);
        if (ccxt->to_read_buf) {
                free(ccxt->to_read_buf->buffer);
                free(ccxt->to_read_buf);
                ccxt->to_read_buf = NULL;
                __sync_sub_and_fetch(&ccxt->read_counter, 1);
        }
        pthread_mutex_unlock(&ccxt->to_read_buf_lock);
        pthread_mutex_destroy(&ccxt->to_read_buf_lock);

        while (ccxt->read_counter) {
                WLOG("Waiting read response, reading counter:%lu.", ccxt->read_counter);
                sleep(1);
        }
        while (ccxt->write_counter != ccxt->write_done_counter) {
                WLOG("Waiting write response, request counter:%lu, response counter:%lu.", ccxt->write_counter, ccxt->write_done_counter);
                sleep(1);
        }

        free(ccxt);
}

static void* getContext(Server *srv) {
        ServerPrivate *priv_p = srv->p;
        return priv_p->param.context;
}

static void destroy(Server* this) {
        ServerPrivate *priv_p = this->p;
        
        priv_p->socket.m->destroy(&priv_p->socket);
        priv_p->tp_read.m->destroy(&priv_p->tp_read);
        priv_p->tp_write.m->destroy(&priv_p->tp_write);
        free(priv_p);
}

static ServerMethod method = {
        .getContext = getContext,
        .destroy = destroy,
};

bool initServer(Server* this, ServerParam* param) {
        ServerPrivate *priv_p = malloc(sizeof(*priv_p));
        bool rc;

        this->p = priv_p;
        this->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));

        ThreadPoolParam param_tp;
        memset(&param_tp, 0, sizeof(param_tp));
        param_tp.do_batch = NULL;
        param_tp.thread_number = param->socket_io_thread_number;
        rc = initThreadPool(&priv_p->tp_read, &param_tp);
        assert(rc);
        rc = initThreadPool(&priv_p->tp_write, &param_tp);
        assert(rc);

        SocketLinuxParam linux_param;
        strcpy(linux_param.super.host, "127.0.0.1");
        linux_param.super.onClose = serverOnClose;
        linux_param.super.onConnect = serverOnConnect;
        linux_param.super.port = param->port;
        linux_param.super.type = SOCKET_TYPE_SERVER;
        linux_param.super.context = this;
        linux_param.read_tp = &priv_p->tp_read;
        linux_param.write_tp = &priv_p->tp_write;

        rc = initSocketLinux(&priv_p->socket, &linux_param);
        assert(rc);
        return true;
}


