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

#include <client/Client.h>
#include <client/sender/RequestSender.h>
#include <Log.h>

typedef struct ClientPrivate {
        ClientParam     param;
        Connection      *conn;
        Socket          socket;
        sem_t           sem;
} ClientPrivate;


static void clientOnClose(Connection *conn) {
        Socket *socket = conn->m->getSocket(conn);
        Client* obj = socket->m->getContext(socket);
        ClientPrivate *priv_p = obj->p;

        priv_p->conn = NULL;
        DLOG("clientOnClose");
}

static void clientOnConnect(Connection *conn) {
        Socket *socket = conn->m->getSocket(conn);
        Client* obj = socket->m->getContext(socket);
        ClientPrivate *priv_p = obj->p;

        priv_p->conn = conn;
        sem_post(&priv_p->sem);
}

typedef struct ClientSendArg {
        char                    *buffer;
        size_t                  buffer_len;
        ClientSendCallback      callback;
        void                    *arg;
} ClientSendArg;

static void clientWriteCallback(Connection *conn, bool rc, void *cbarg) {
        if (rc == false) {
                ELOG("Write failed!");
                // TODO
        } else {
                DLOG("Client write success\n");
        }
}

static bool clientSendRequest(Client* obj, Request* req, ClientSendCallback callback, void *arg, bool *free_req) {
        ClientPrivate *priv_p = obj->p;
        Connection *conn = priv_p->conn;

        if (conn == NULL) {
                return false;
        }
        RequestSender *sender = &priv_p->param.request_sender[req->resource_id];
        RequestEncoder encoder = sender->request_encoders[req->request_id];
        ClientSendArg *send_arg = malloc(sizeof(*send_arg));
        send_arg->callback = callback;
        send_arg->arg = arg;
        //TODO
        bool rc = encoder(req, &send_arg->buffer, &send_arg->buffer_len, free_req);
        if (rc == false) {
                *free_req = true;
                free(send_arg);
                return rc;
        }
        rc = conn->m->write(conn, send_arg->buffer, send_arg->buffer_len, clientWriteCallback, send_arg);
        if (rc) {
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
        priv_p->socket.m->destroy(&priv_p->socket);
        free(priv_p);
}

static ClientMethod method = {
        .sendRequest = clientSendRequest,
        .destroy = destroy,
};

bool initClient(Client* obj, ClientParam* param) {
        ClientPrivate *priv_p = malloc(sizeof(*priv_p));
        int rc;

        obj->p = priv_p;
        obj->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));
        priv_p->conn = NULL;
        sem_init(&priv_p->sem, 0, 0);
        
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


