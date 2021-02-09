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

RequestEncoder SampleRequestEncoder[] = {
                SampleRequestEncoderGet,
                SampleRequestEncoderPut,
                SampleRequestEncoderList,
};

bool SampleRequestEncoderGet(Request *req, char **buffer, size_t *buff_len, bool *free_req) {
	SampleGetRequest *req1 = (SampleGetRequest*)req;
	*buffer = (char*) req1;
	*buff_len = sizeof(*req1);
	*free_req = false;
        return true;
}

bool SampleRequestEncoderPut(Request *req, char **buffer, size_t *buff_len, bool *free_req) {
	SamplePutRequest *req1 = (SamplePutRequest*)req;
	*buffer = (char*) req1;
	*buff_len = sizeof(*req1);
	*free_req = false;
        return true;
}

bool SampleRequestEncoderList(Request *req, char **buffer, size_t *buff_len, bool *free_req) {
	SampleListRequest *req1 = (SampleListRequest*)req;
	*buffer = (char*) req1;
	*buff_len = sizeof(*req1);
	*free_req = false;
        return true;
}

ResponseDecoder SampleResponseDecoder[] = {
                SampleResponseDecoderGet,
                SampleResponseDecoderPut,
                SampleResponseDecoderList,
};

Response* SampleResponseDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp) {
	SampleGetResponse *resp = (SampleGetResponse*)buffer;
	size_t len = sizeof(*resp);
	if (buff_len < len) return NULL;
	len += resp->sample.path_length;
	if (buff_len < len) return NULL;
	*consume_len = len;
	*free_resp = false;
	return &resp->super;
}

Response* SampleResponseDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp) {
	SamplePutResponse *resp = (SamplePutResponse*)buffer;
	size_t len = sizeof(*resp);
	if (buff_len < len) return NULL;
	*consume_len = len;
	*free_resp = false;
        return &resp->super;
}

Response* SampleResponseDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp) {
	SampleListResponse *resp = malloc(sizeof(*resp));
	listHeadInit(&resp->sample_head);

	size_t len = 0;
	len += 1; if (buff_len < len) goto err_out;
	resp->super.error_id = *(int8_t*)buffer;
	buffer += 1;

	len += 4; if (buff_len < len) goto err_out;
	resp->super.sequence = *(uint32_t*)buffer;
	buffer += 4;

	len += 4; if (buff_len < len) goto err_out;
	resp->length = *(uint32_t*)buffer;
	buffer += 4;

	int i;
	for (i = 0; i < resp->length; i++) {
		len += sizeof(Sample);
		if (buff_len < len) goto err_out;
		Sample *s = (Sample*)buffer;
		len += s->path_length;
		if (buff_len < len) goto err_out;
		buffer += sizeof(Sample) + s->path_length;
		listAddTail(&s->element, &resp->sample_head);
	}
	*consume_len = len;
	*free_resp = true;
        return &resp->super;
err_out:
	free(resp);
	return NULL;
}



