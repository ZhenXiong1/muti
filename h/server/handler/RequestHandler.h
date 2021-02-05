/*
 * Resource.h
 *
 *  Created on: Jan 8, 2021
 *      Author: Zhen Xiong
 */

#ifndef SERVER_HANDLER_REQUESTHANDLER_H_
#define SERVER_HANDLER_REQUESTHANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include <util/ThreadPool.h>
#include <network/Connection.h>
#include <res/Resource.h>

typedef struct Readbuffer {
        char                    *buffer;
        off_t                   read_buffer_start;
        volatile uint32_t       running_request_counter;
} Readbuffer;

typedef struct RequestHandler RequestHandler;
typedef struct RequestW RequestW;
typedef void (*ActionCallback)(RequestW *reqw, uint8_t error_id);

struct RequestW {
        Job             job;
        Request         *request;
        bool            free_req;
        Connection      *connection;
        RequestHandler  *req_handler;
        Response        *response;
        Readbuffer      *read_buffer;
        ActionCallback  action_callback;
};

typedef void (*Action)(RequestW *);
typedef Request* (*RequestDecoder)(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
typedef bool (*ResponseEncoder)(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);

struct RequestHandler {
        Action          *actions;
        RequestDecoder   **request_decoders;
        ResponseEncoder  **response_encoders;
};

#endif /* SERVER_HANDLER_REQUESTHANDLER_H_ */
