/*
 * SampleRequestSender.c
 *
 *  Created on: Jan 12, 2021
 *      Author: Zhen Xiong
 */
#include <string.h>

#include <client/sender/SampleRequestSender.h>
#include <stdbool.h>
#include <res/Sample.h>
#include <server/Server.h>
#include <server/ServerContext.h>
#include <network/Connection.h>
#include <network/Socket.h>

RequestEncoder SampleRequestEncoder[] = {
                SampleRequestEncoderGet,
                SampleRequestEncoderPut,
                SampleRequestEncoderList,
};

bool SampleRequestEncoderGet(Request *req, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)req;
        *buff_len = sizeof(SampleGetRequest);
        *free_resp = false;
        return true;
}

bool SampleRequestEncoderPut(Request *req, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)req;
        SamplePutRequest *get_req = (SamplePutRequest*)req;
        *buff_len = sizeof(SamplePutRequest) + get_req->sample.path_length;
        *free_resp = false;
        return true;
}

bool SampleRequestEncoderList(Request *req, char **buffer, size_t *buff_len, bool *free_resp) {
        *buffer = (char*)req;
        *buff_len = sizeof(SampleListRequest);
        *free_resp = false;
        return true;
}

ResponseDecoder SampleResponseDecoder[] = {
                SampleResponseDecoderGet,
                SampleResponseDecoderPut,
                SampleResponseDecoderList,
};

Response* SampleResponseDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        SampleGetResponse *req = (SampleGetResponse*)buffer;
        req_len = sizeof(SampleGetResponse) + req->sample.path_length;
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Response* SampleResponseDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        size_t req_len;
        SamplePutResponse *req = (SamplePutResponse*)buffer;
        req_len = sizeof(SamplePutResponse);
        if (buff_len < req_len) return NULL;
        *consume_len = req_len;
        *free_req = false;
        return &req->super;
}

Response* SampleResponseDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        SampleListResponse *list_resp = malloc(sizeof(SampleListResponse));
        size_t clen = 0;

        clen += 1; if (buff_len < clen) { free(list_resp); return NULL;}
        list_resp->super.error_id = *(int8_t*)buffer;
        buffer += 1;

        clen += 4; if (buff_len < clen) { free(list_resp); return NULL;}
        list_resp->super.sequence = *(uint32_t*)buffer;
        buffer += 4;

        clen += 4; if (buff_len < clen) { free(list_resp); return NULL;}
        list_resp->length = *(uint32_t*)buffer;
        buffer += 4;

        int i;
        ListHead *head = list_resp->sample_list;
        listHeadInit(head);
        for (i = 0; i < list_resp->length; i++) {
                Sample *s = buffer;
                size_t blen = sizeof(*s) + s->path_length;
                clen += blen; if (buff_len < clen) { free(list_resp); return NULL;}
                buffer += blen;
                listAddTail(&s->element, head);
        }
        *consume_len = clen;
        *free_req = true;
        return &list_resp->super;
}



