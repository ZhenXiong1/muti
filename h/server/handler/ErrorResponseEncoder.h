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

bool ErrorResponseEncoder(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);

#endif /* SERVER_HANDLER_ERRORRESPONSEENCODER_H_ */
