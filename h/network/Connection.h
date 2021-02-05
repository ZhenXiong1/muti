/*
 * Connection.h
 *
 *  Created on: 2020/12/9
 *      Author: Rick
 */

#ifndef NETWORK_CONNECTION_H_
#define NETWORK_CONNECTION_H_

#include "Object.h"

typedef struct Socket Socket;
typedef struct Connection Connection;
typedef void (*ConnectionReadCallback)(Connection*, bool rc, void* buffer, size_t sz, void *cbarg);
typedef void (*ConnectionWriteCallback)(Connection*, bool rc, void *cbarg);

typedef struct ConnectionMethod {
        bool    (*read)(Connection*, void *buf, size_t, ConnectionReadCallback, void *cbarg);
        bool    (*write)(Connection*, void *buf, size_t, ConnectionWriteCallback, void *cbarg);
        void    (*close)(Connection*);
        Socket* (*getSocket)(Connection*);
        void    (*setContext)(Connection*, void *);
        void*   (*getContext)(Connection*);
} ConnectionMethod;

struct Connection {
        void                    *p;
        ConnectionMethod        *m;
};

#endif /* NETWORK_CONNECTION_H_ */
