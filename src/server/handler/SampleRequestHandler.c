/*
 * Sample.c
 *
 *  Created on: Jan 12, 2021
 *      Author: root
 */
#include <string.h>

#include <server/handler/SampleRequestHandler.h>
#include <stdbool.h>
#include <res/Sample.h>
#include <server/dao/SampleDao.h>
#include <server/Server.h>
#include <server/ServerContext.h>
#include <network/Connection.h>
#include <network/Socket.h>

Action SampleActions[] = {
        SampleActionGet,
        SampleActionPut,
        SampleActionList,
};

void SampleActionGet(SRequest *req) {
        SampleGetRequest *request = (SampleGetRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContext *sctx = server->m->getContext(server);
        SampleDao *sdao = &sctx->sampleDao;

        Sample* sample = sdao->m->getSample(sdao, request->id);
        Response *resp;
        if (sample == NULL) {
                resp = malloc(sizeof(*resp));
                resp->response_type = request->super.request_type;
                resp->error_id = 1;
        } else {
                SampleGetResponse *resp1 = malloc(sizeof(*resp) + sample->path_length);
                resp = &resp1->super;
                resp->response_type = request->super.request_type;
                resp->error_id = 0;
                memcpy(&resp1->sample, sample, sizeof(*sample) + sample->path_length);
        }
        req->response = resp;
        req->action_callback(req, resp->error_id);
}

void SampleActionPut(SRequest *req) {
        SamplePutRequest *request = (SamplePutRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContext *sctx = server->m->getContext(server);
        SampleDao *sdao = &sctx->sampleDao;
        Sample* sample = &request->sample;
        Response *resp;

        resp = malloc(sizeof(*resp));
        resp->response_type = request->super.request_type;
        resp->error_id = (int8_t)sdao->m->putSample(sdao, sample);

        req->response = resp;
        req->action_callback(req, resp->error_id);
}

void SampleActionList(SRequest *req) {
        SampleListRequest *request = (SampleListRequest*) req->request;
        Socket* socket = req->connection->m->getSocket(req->connection);
        Server* server = socket->m->getContext(socket);
        ServerContext *sctx = server->m->getContext(server);
        SampleDao *sdao = &sctx->sampleDao;
        SampleListResponse *resp = malloc(sizeof(*resp));

        resp->length = sdao->m->listSample(sdao, &resp->sample_list, request->page, request->page_size);

        resp->super.response_type = request->super.request_type;
        resp->super.error_id = 0;

        req->response = &resp->super;
        req->action_callback(req, resp->super.error_id);
}

static RequestDecoder SampleRequestDecoderC[] = {
        SampleRequestDecoderCGet,
        SampleRequestDecoderCPut,
        SampleRequestDecoderCList,
};

RequestDecoder* SampleRequestDecoder[] = {
        SampleRequestDecoderC,
        NULL,
        NULL,
};

Request* SampleRequestDecoderCGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        SampleGetRequest *req = (SampleGetRequest*)buffer;
        req_len = sizeof(SampleGetRequest);
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Request* SampleRequestDecoderCPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        SamplePutRequest *req = (SamplePutRequest*)buffer;
        req_len = sizeof(SamplePutRequest) + req->sample.path_length;
        req->sample.path = req->sample.data;
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Request* SampleRequestDecoderCList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        SampleListRequest *req = (SampleListRequest*)buffer;
        req_len = sizeof(SampleListRequest);
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

static ResponseEncoder SampleResponseEncoderC[] = {
        SampleResponseEncoderCGet,
        SampleResponseEncoderCPut,
        SampleResponseEncoderCList,
};

ResponseEncoder* SampleResponseEncoder[] = {
        SampleResponseEncoderC,
        NULL,
        NULL,
};

bool SampleResponseEncoderCGet(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)resp;
        SampleGetResponse *get_resp = (SampleGetResponse*)resp;
        *buff_len = sizeof(SampleGetResponse) + get_resp->sample.path_length;
        *free_resp = false;
        return true;
}

bool SampleResponseEncoderCPut(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)resp;
        *buff_len = sizeof(SamplePutResponse);
        *free_resp = false;
        return true;
}

bool SampleResponseEncoderCList(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        SampleListResponse *list_resp = (SampleListResponse*)resp;
        size_t buf_len = 0;
        char *buf;

        buf = malloc(sizeof(Response) + sizeof(list_resp->length) + list_resp->length * (sizeof(Sample) + 1024));
        *buffer = buf;

        *(uint8_t*)buf = resp->response_type;
        buf += 1;
        buf_len += 1;

        *(int8_t*)buf = resp->error_id;
        buf += 1;
        buf_len += 1;

        *(uint32_t*)buf = list_resp->length;
        buf += sizeof(uint32_t);
        buf_len += sizeof(uint32_t);

        int i;
        ListHead *head = list_resp->sample_list;
        for (i = 0; i < list_resp->length; i++) {
                Sample *s = listFirstEntry(head, Sample, element);
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

