/*
 * RequestSender.h
 *
 *  Created on: Feb 6, 2021
 *      Author: Zhen Xiong
 */

#ifndef CLIENT_SENDER_REQUESTSENDER_H_
#define CLIENT_SENDER_REQUESTSENDER_H_

#include <stdint.h>
#include <stdbool.h>

#include <res/Resource.h>

typedef struct RequestSender RequestSender;

typedef Response* (*ResponseDecoder)(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp);
typedef bool (*RequestEncoder)(Request *req, char **buffer, size_t *buff_len, bool *free_req);

struct RequestSender {
        int             id;
        RequestEncoder  *request_encoders;
        ResponseDecoder *response_decoders;
};

extern Response* ErrorResponseDecoder(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp);

#endif /* CLIENT_SENDER_REQUESTSENDER_H_ */
