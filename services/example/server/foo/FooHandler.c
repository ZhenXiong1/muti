/*
 * Foo.c
 *
 *  Created on: Jan 12, 2021
 *      Author: root
 */
#include <example/server/foo/FooHandler.h>
#include <string.h>

#include <stdbool.h>
#include <example/share/Foo.h>
#include "dao/FooDao.h"
#include <server/Server.h>
#include "../ServerContextExample.h"
#include <network/Connection.h>
#include <network/Socket.h>
#include <Log.h>

Action FooActions[] = {
        FooActionGet,
        FooActionPut,
        FooActionList,
};

void FooActionGet(SRequest *req) {
        FooGetRequest *request = (FooGetRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContextExample *sctx = server->m->getContext(server);
        FooDao *sdao = &sctx->fooDao;

        Foo* foo = sdao->m->getFoo(sdao, request->id);
        Response *resp;
        if (foo == NULL) {
                resp = calloc(1, sizeof(*resp));
                resp->sequence = request->super.sequence;
                resp->error_id = 1;
        } else {
                FooGetResponse *resp1 = calloc(1, sizeof(*resp1) + foo->path_length);
                resp = &resp1->super;
                resp->sequence = request->super.sequence;
                resp->error_id = 0;
                memcpy(&resp1->foo, foo, sizeof(*foo) + foo->path_length);
        }
        req->response = resp;
        req->action_callback(req);
}

void FooActionPut(SRequest *req) {
        FooPutRequest *request = (FooPutRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContextExample *sctx = server->m->getContext(server);
        FooDao *sdao = &sctx->fooDao;
        Foo* foo = &request->foo;
        Response *resp;

        resp = calloc(1, sizeof(*resp));
        resp->sequence = request->super.sequence;
        resp->error_id = (int8_t)sdao->m->putFoo(sdao, foo);

        req->response = resp;
        req->action_callback(req);
}

void FooActionList(SRequest *req) {
        FooListRequest *request = (FooListRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContextExample *sctx = server->m->getContext(server);
        FooDao *sdao = &sctx->fooDao;
        FooListResponse *resp = malloc(sizeof(*resp));

        resp->length = sdao->m->listFoo(sdao, &resp->foo_list, request->page, request->page_size);

        resp->super.sequence = request->super.sequence;
        resp->super.error_id = 0;

        req->response = &resp->super;
        req->action_callback(req);
}

RequestDecoder FooRequestDecoder[] = {
        FooRequestDecoderGet,
        FooRequestDecoderPut,
        FooRequestDecoderList,
};

Request* FooRequestDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        FooGetRequest *req = (FooGetRequest*)buffer;
        req_len = sizeof(FooGetRequest);
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Request* FooRequestDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        FooPutRequest *req = (FooPutRequest*)buffer;
        req_len = sizeof(FooPutRequest);
        if (buff_len < req_len) return NULL;
        req_len += req->foo.path_length;
        if (buff_len < req_len) return NULL;
        req->foo.path = req->foo.data;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Request* FooRequestDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        FooListRequest *req = (FooListRequest*)buffer;
        req_len = sizeof(FooListRequest);
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

ResponseEncoder FooResponseEncoder[] = {
        FooResponseEncoderGet,
        FooResponseEncoderPut,
        FooResponseEncoderList,
};

bool FooResponseEncoderGet(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)resp;
        FooGetResponse *get_resp = (FooGetResponse*)resp;
        *buff_len = sizeof(FooGetResponse) + get_resp->foo.path_length;
        *free_resp = false;
        return true;
}

bool FooResponseEncoderPut(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)resp;
        *buff_len = sizeof(FooPutResponse);
        *free_resp = false;
        return true;
}

bool FooResponseEncoderList(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        FooListResponse *list_resp = (FooListResponse*)resp;
        size_t buf_len = 0;
        char *buf;

        buf = malloc(sizeof(Response) + sizeof(list_resp->length) + list_resp->length * (sizeof(Foo) + 1024));
        *buffer = buf;

        *(int8_t*)buf = resp->error_id;
        buf += 1;
        buf_len += 1;

        *(uint32_t*)buf = resp->sequence;
        buf += 4;
        buf_len += 4;

        *(uint32_t*)buf = list_resp->length;
        buf += sizeof(uint32_t);
        buf_len += sizeof(uint32_t);

        int i;
        ListHead *head = list_resp->foo_list;
        for (i = 0; i < list_resp->length; i++) {
                Foo *s = listFirstEntry(head, Foo, element);
                size_t blen = sizeof(*s) + s->path_length;
                memcpy(buf, s, blen);
                buf += blen;
                buf_len += blen;
                head = &s->element;
        }

        *buff_len = buf_len;
        *free_resp = true;
        return true;
}

