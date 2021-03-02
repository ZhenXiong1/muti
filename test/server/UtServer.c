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
#include <server/RequestHandlers.h>
#include <server/sample/ServerSampleContext.h>

int UtServer(int argv, char **argvs) {
        Server server;
        ServerParam param;
        ThreadPool work_tp;
        ThreadPoolParam param_tp;
        ServerSampleContext scxt;
        ServerSampleContextParam param_cxt;

        param_tp.do_batch = NULL;
        param_tp.thread_number = 6;

        bool rc = initThreadPool(&work_tp, &param_tp);
        assert(rc == true);

        rc = initServerSampleContext(&scxt, &param_cxt);
        assert(rc == true);

        param.port = 10809;
        param.read_buffer_size = 1 << 12;
        param.request_handler = ServerRequestHandlers;
        param.request_handler_length = sizeof(ServerRequestHandlers) / sizeof(RequestHandler);
        param.socket_io_thread_number = 1;
        param.worker_tp = &work_tp;
        param.max_read_buffer_counter = 1000000;
        param.context = &scxt;

        rc = initServer(&server, &param);
        assert(rc == true);
        sleep(100);
        DLOG("Exiting...");
        server.m->destroy(&server);
        scxt.m->destroy(&scxt);
        work_tp.m->destroy(&work_tp);

        return 0;
}


