/*
 * Server.h
 *
 *  Created on: Jan 15, 2021
 *      Author: root
 */

#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_
#include <server/RequestHandler.h>
#include <stdbool.h>
#include <util/ThreadPool.h>

typedef struct Server Server;
typedef struct ServerMethod {
        void*           (*getContext)(Server *);
        void            (*destroy)(Server*);
} ServerMethod;

struct Server {
        void              *p;
        ServerMethod      *m;
};

typedef struct ServerParam {
        RequestHandler  *request_handler;
        uint16_t        request_handler_length;
        void            *context;
        ThreadPool      *worker_tp;
        int             port;
        size_t          read_buffer_size;
        int             max_read_buffer_counter;
        int             socket_io_thread_number;
} ServerParam;

bool initServer(Server*, ServerParam*);

#endif /* SERVER_SERVER_H_ */
