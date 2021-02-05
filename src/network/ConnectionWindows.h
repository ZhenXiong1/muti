/*
 * ConnectionWindows.h
 *
 *  Created on: 2020\12\12
 *      Author: Rick
 */

#ifndef NETWORK_CONNECTIONWINDOWS_H_
#define NETWORK_CONNECTIONWINDOWS_H_

#if defined(WIN32) || defined(WIN64)

#include "network/Connection.h"
#include "network/Socket.h"
#include "util/LinkedList.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#define mkdir(path, permission) mkdir(path)
#ifndef WIN64
#define usleep(n) Sleep((n)<1000?1:((n)/1000))
#endif
#define PRSZst   "lu"
#define sleep(n) Sleep(n * 1000)

typedef enum IOOperateType {
    IO_OPERATE_TYPE_READ,
    IO_OPERATE_TYPE_WRITE,
    IO_OPERATE_TYPE_STOP,
    IO_OPERATE_TYPE_CLOSE,
} IOOperateType;

typedef struct IOContext {
    OVERLAPPED          overlap;
    WSABUF              wsabuf;
    DWORD               transNum;
    IOOperateType       type;
    Connection          *conn;
    union {
            ConnectionReadCallback readCallback;
            ConnectionWriteCallback writeCallback;
    };
    void                *cb_arg;
} IOContext;

typedef struct ConnectionWin {
        Connection      super;
        Socket          *socket;
        ListElement     element;
        volatile int    ref;
        volatile int    is_closed;
} ConnectionWin;

typedef struct ConnectionWindowsMethod {
        ConnectionMethod        super;
        void                    (*destroy)(Connection*);
        void                    (*close)(Connection*);
} ConnectionWindowsMethod;

typedef struct ConnectionParam {
        int     fd;
} ConnectionParam;

bool initConnectionWindows(Connection*, ConnectionParam*);

#endif

#endif /* NETWORK_CONNECTIONWINDOWS_H_ */
