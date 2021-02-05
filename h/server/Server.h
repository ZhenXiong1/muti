/*
 * Server.h
 *
 *  Created on: Jan 15, 2021
 *      Author: root
 */

#ifndef SERVER_SERVER_H_
#define SERVER_SERVER_H_
#include <server/handler/RequestHandler.h>
#include <stdbool.h>
#include <util/ThreadPool.h>
#include <server/ServerContext.h>

typedef struct Server Server;
typedef struct ServerMethod {
        ServerContext*  (*getContext)(Server *);
        void            (*destroy)(Server*);
} ServerMethod;

struct Server {
        void              *p;
        ServerMethod      *m;
};

typedef struct ServerParam {
        RequestHandler        *resource;
        uint8_t         resource_length;
        ThreadPool      *worker_tp;
        int             port;
        size_t          read_buffer_size;
} ServerParam;

bool initServer(Server*, ServerParam*);

#endif /* SERVER_SERVER_H_ */
