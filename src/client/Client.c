/*
 * Client.c
 *
 *  Created on: Feb 6, 2021
 *      Author: Zhen Xiong
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include <client/Client.h>
#include <client/sender/RequestSender.h>
#include <Log.h>
#include <util/LinkedList.h>

#define CLIENT_WAITING_HASH_LEN 64

typedef struct ClientPrivate {
        ClientParam             param;
        Connection              *conn;
        Socket                  socket;
        sem_t                   sem;
        volatile uint32_t       sequence;
        pthread_mutex_t         waiting_hash_locks[CLIENT_WAITING_HASH_LEN];
        ListHead                waiting_hash_slots[CLIENT_WAITING_HASH_LEN];
} ClientPrivate;

typedef struct CConnectionContext {
        volatile uint64_t       read_counter;
        volatile uint64_t       read_done_counter;
        volatile uint64_t       write_counter;
        volatile uint64_t       write_done_counter;
} CConnectionContext;

typedef struct ClientSendArg {
        ListElement             element;
        uint32_t                sequence;
        char                    *buffer;
        size_t                  buffer_len;
        ClientSendCallback      callback;
        void                    *arg;
} ClientSendArg;

static void clientOnClose(Connection *conn) {
        Socket *socket = conn->m->getSocket(conn);
        CConnectionContext *ccxt = conn->m->getContext(conn);
        Client* obj = socket->m->getContext(socket);
        ClientPrivate *priv_p = obj->p;
        ListHead *slot;
        pthread_mutex_t *lock;
        ClientSendArg *arg, *arg1;
        Response err_resp;
        int i;

        priv_p->conn = NULL;

        while (ccxt->read_counter != ccxt->read_done_counter) {
                WLOG("Waiting read response, request counter:%lu, response counter:%lu.", ccxt->read_counter, ccxt->read_done_counter);
                sleep(1);
        }
        while (ccxt->write_counter != ccxt->write_done_counter) {
                WLOG("Waiting write response, request counter:%lu, response counter:%lu.", ccxt->write_counter, ccxt->write_done_counter);
                sleep(1);
        }
        free(ccxt);

        err_resp.error_id = -4;
        err_resp.sequence = 0;
        for (i = 0; i < CLIENT_WAITING_HASH_LEN; i++) {
                slot = &priv_p->waiting_hash_slots[i];
                lock = &priv_p->waiting_hash_locks[i];
                pthread_mutex_lock(lock);
                listForEachEntrySafe(arg, arg1, slot, element) {
                        listDel(&arg->element);
                        arg->callback(obj, &err_resp, arg->arg);
                        if (arg->buffer) {
                                free(arg->buffer);
                                arg->buffer = NULL;
                        }
                        free(arg);
                }
                pthread_mutex_unlock(lock);
        }
        DLOG("clientOnClose");
}

static void clientOnConnect(Connection *conn) {
        Socket *socket = conn->m->getSocket(conn);
        Client* obj = socket->m->getContext(socket);
        CConnectionContext *ctx = calloc(1, sizeof(CConnectionContext));
        ClientPrivate *priv_p = obj->p;
        priv_p->conn = conn;
        conn->m->setContext(conn, ctx);

        sem_post(&priv_p->sem);
        // TODO start read job
}

static void clientWriteCallback(Connection *conn, bool rc, void *cbarg) {
        Socket *socket = conn->m->getSocket(conn);
        Client *obj = socket->m->getContext(socket);
        CConnectionContext *ccxt = conn->m->getContext(conn);
        pthread_mutex_t *lock;
        ClientPrivate *priv_p = obj->p;
        ClientSendArg *send_arg = cbarg;

        if (rc == false) {
                ELOG("Write failed!");

                uint32_t hash = send_arg->sequence & (CLIENT_WAITING_HASH_LEN - 1);
                lock = &priv_p->waiting_hash_locks[hash];
                pthread_mutex_lock(lock);
                listDel(&send_arg->element);
                pthread_mutex_unlock(lock);

                Response resp;
                resp.error_id = -3;
                send_arg->callback(obj, &resp, cbarg);
                free(send_arg->buffer);
                free(send_arg);
        } else {
                DLOG("Client write success\n");
                free(send_arg->buffer);
                send_arg->buffer = NULL;
        }
        __sync_add_and_fetch(&ccxt->write_done_counter, 1);
}

static bool clientSendRequest(Client* obj, Request* req, ClientSendCallback callback, void *arg, bool *free_req) {
        ClientPrivate *priv_p = obj->p;
        Connection *conn = priv_p->conn;
        if (conn == NULL) {
                return false;
        }

        CConnectionContext *ccxt = conn->m->getContext(conn);
        RequestSender *sender = &priv_p->param.request_sender[req->resource_id];
        RequestEncoder encoder = sender->request_encoders[req->request_id];
        ClientSendArg *send_arg = malloc(sizeof(*send_arg));

        send_arg->callback = callback;
        send_arg->arg = arg;
        req->sequence = __sync_fetch_and_add(&priv_p->sequence, 1);
        send_arg->sequence = req->sequence;

        bool rc = encoder(req, &send_arg->buffer, &send_arg->buffer_len, free_req);
        if (rc == false) {
                *free_req = true;
                free(send_arg);
                return rc;
        }

        uint32_t hash = send_arg->sequence & (CLIENT_WAITING_HASH_LEN - 1);
        pthread_mutex_t *lock = &priv_p->waiting_hash_locks[hash];
        ListHead *slot = &priv_p->waiting_hash_slots[hash];
        pthread_mutex_lock(lock);
        listAddTail(&send_arg->element, slot);
        pthread_mutex_unlock(lock);
        __sync_add_and_fetch(&ccxt->write_counter, 1);

        rc = conn->m->write(conn, send_arg->buffer, send_arg->buffer_len, clientWriteCallback, send_arg);
        if (rc) {
                __sync_add_and_fetch(&ccxt->write_done_counter, 1);
                pthread_mutex_lock(lock);
                listDel(&send_arg->element);
                pthread_mutex_unlock(lock);

                if (*free_req) {
                        free(send_arg->buffer);
                }
                *free_req = true;
                free(send_arg);
        }
        return rc;
}

static void destroy(Client* obj) {
        ClientPrivate *priv_p = obj->p;
        int i;

        for (i = 0; i < CLIENT_WAITING_HASH_LEN; i++) {
                pthread_mutex_t *lock;
                lock = &priv_p->waiting_hash_locks[i];
                pthread_mutex_destroy(lock);
        }

        priv_p->socket.m->destroy(&priv_p->socket);
        free(priv_p);
}

static ClientMethod method = {
        .sendRequest = clientSendRequest,
        .destroy = destroy,
};

bool initClient(Client* obj, ClientParam* param) {
        ClientPrivate *priv_p = malloc(sizeof(*priv_p));
        ListHead *wlist;
        pthread_mutex_t *lock;
        int i, rc;

        obj->p = priv_p;
        obj->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));
        priv_p->conn = NULL;
        sem_init(&priv_p->sem, 0, 0);
        priv_p->sequence = 0;
        for (i = 0; i < CLIENT_WAITING_HASH_LEN; i++) {
                wlist = &priv_p->waiting_hash_slots[i];
                lock = &priv_p->waiting_hash_locks[i];
                pthread_mutex_init(lock, NULL);
                listHeadInit(wlist);
        }

        SocketLinuxParam sparam;
        strcpy(sparam.super.host, param->host);
        sparam.super.onClose = clientOnClose;
        sparam.super.onConnect = clientOnConnect;
        sparam.super.port = param->port;
        sparam.super.type = SOCKET_TYPE_CLIENT;
        sparam.read_tp = param->read_tp;
        sparam.write_tp = param->write_tp;
        sparam.super.context = obj;
        rc = initSocketLinux(&priv_p->socket, &sparam);
        if (rc == false) {
                free(priv_p);
                goto out;
        }
        sem_wait(&priv_p->sem);
out:
        return rc;
}


