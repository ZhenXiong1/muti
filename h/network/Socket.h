/*
 * Socket.h
 *
 *  Created on: 2020/12/8
 *      Author: Rick
 */

#ifndef NETWORK_SOCKET_H_
#define NETWORK_SOCKET_H_

#include "Object.h"
#include "Connection.h"
#include "util/ThreadPool.h"

#define NETWORK_HOST_LEN  127

typedef enum SocketType {
        SOCKET_TYPE_SERVER = 1,
        SOCKET_TYPE_CLIENT
} SocketType;

typedef enum SocketIPType {
        SOCKET_IP_TYPE_IPV4 = 1,
        SOCKET_IP_TYPE_IPV6
} SocketIPType;

typedef void (*OnConnect)(Connection *);
typedef void (*OnClose)(Connection *);

typedef struct SocketMethod {
        void*           (*getContext)(Socket*);
        void	        (*destroy)(Socket*);
} SocketMethod;

struct Socket {
        void            *p;
        SocketMethod    *m;
};

typedef struct SocketParam {
        SocketType      type;
        char            host[NETWORK_HOST_LEN + 1];
        int             port;
        int             working_thread_num;
        void            *context;
        OnConnect       onConnect;
        OnClose         onClose;        // Release and close all resources related the connection, as it will be freed.
} SocketParam;

typedef struct SocketLinuxParam {
        SocketParam     super;
        ThreadPool      *read_tp;
        ThreadPool      *write_tp;
} SocketLinuxParam;

bool initSocketWindows(Socket*, SocketParam*);
bool initSocketLinux(Socket*, SocketLinuxParam*);

#endif /* NETWORK_SOCKET_H_ */
