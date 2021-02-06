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

bool SampleRequestEncoderGet(Request *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        return true;
}

bool SampleRequestEncoderPut(Request *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        return true;
}

bool SampleRequestEncoderList(Request *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        return true;
}

ResponseDecoder SampleResponseDecoder[] = {
                SampleResponseDecoderGet,
                SampleResponseDecoderPut,
                SampleResponseDecoderList,
};

Response* SampleResponseDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        return NULL;
}

Response* SampleResponseDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        return NULL;
}

Response* SampleResponseDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req) {
        return NULL;
}



