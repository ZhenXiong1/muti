/*
 * ConnectionLinux.h
 *
 *  Created on: 2020\12\12
 *      Author: Rick
 */

#ifndef NETWORK_CONNECTIONLINUX_H_
#define NETWORK_CONNECTIONLINUX_H_

#include <pthread.h>
#include <stdbool.h>
#include "network/Connection.h"
#include "util/ThreadPool.h"
#include "network/Socket.h"

typedef struct ConnectionLinux {
        Connection              super;
        int                     fd;
        Job                     write_job;
        Job                     read_job;
        volatile int            is_writing;
        volatile int            is_reading;
        ListHead                write_jobs_head;
        pthread_spinlock_t      write_jobs_head_lck;
        ListHead                read_jobs_head;
        pthread_spinlock_t      read_jobs_head_lck;
        Socket                  *socket;
        ListElement             element;
        volatile int            ref;
        volatile int            is_closed;
} ConnectionLinux;

typedef struct IOContext {
        ListElement             element;
        char                    *buf;
        size_t                  buf_size;
        size_t                  trans_num;
        Connection              *conn;
        union {
                ConnectionReadCallback  readCallback;
                ConnectionWriteCallback writeCallback;
        };
        void                    *cb_arg;
        bool                    rc;
} IOContext;

typedef struct ConnectionLinuxMethod {
        ConnectionMethod        super;
        void                    (*destroy)(Connection*);
} ConnectionLinuxMethod;

typedef struct ConnectionLinuxParam {
        int     fd;
} ConnectionLinuxParam;

bool initConnectionLinux(Connection*, ConnectionLinuxParam*);


#endif /* NETWORK_CONNECTIONLINUX_H_ */
