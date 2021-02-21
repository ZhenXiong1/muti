/*
 * UtServer.c
 *
 *  Created on: Feb 18, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <Log.h>
#include <server/Server.h>
#include <util/ThreadPool.h>
#include <server/handler/RequestHandlers.h>

int UtServer(int argv, char **argvs) {
        Server server;
        ServerParam param;
        ThreadPool work_tp;
        ThreadPoolParam param_tp;
        param_tp.do_batch = NULL;
        param_tp.thread_number = 6;

        bool rc = initThreadPool(&work_tp, &param_tp);
        assert(rc == true);
        param.port = 10809;
        param.read_buffer_size = 1 << 11;
        param.request_handler = ServerRequestHandlers;
        param.request_handler_length = sizeof(ServerRequestHandlers);
        DLOG("param.request_handler_length:%u", param.request_handler_length);
        param.socket_io_thread_number = 6;
        param.worker_tp = &work_tp;
        param.max_handle_request = 1000000;

        rc = initServer(&server, &param);
        assert(rc == true);
        sleep(100);
        DLOG("Exiting...");
        server.m->destroy(&server);
        work_tp.m->destroy(&work_tp);

        return 0;
}


