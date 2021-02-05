/*
 * ErrorResponseEncoder.h
 *
 *  Created on: Jan 24, 2021
 *      Author: root
 */

#ifndef SERVER_HANDLER_ERRORRESPONSEENCODER_H_
#define SERVER_HANDLER_ERRORRESPONSEENCODER_H_
#include <server/handler/RequestHandler.h>
#include <stdbool.h>

bool ErrorResponseEncoderC(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool ErrorResponseEncoderJava(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool ErrorResponseEncoderHttp(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);

extern ResponseEncoder errorResponseEncoder[];

#endif /* SERVER_HANDLER_ERRORRESPONSEENCODER_H_ */
