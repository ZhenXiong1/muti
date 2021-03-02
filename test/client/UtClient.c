/*
 * UtClient.c
 *
 *  Created on: Feb 18, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <semaphore.h>

#include <Log.h>
#include <client/Client.h>
#include <util/ThreadPool.h>
#include <client/RequestSenders.h>
#include <share/Sample.h>

static void UtClientSamplePutSendCallback(Client *client, Response *resp, void *arg) {
        if (resp->error_id) {
//                ELOG("Send request failed.");
        }
        volatile int *done_number = arg;
        __sync_add_and_fetch(done_number, 1);
}

int UtClient(int argv, char **argvs) {
        Client client;
        ClientParam param;
        ThreadPool work_tp, write_tp, read_tp;
        ThreadPoolParam param_tp;

        param_tp.do_batch = NULL;
        param_tp.thread_number = 0;

        bool rc = initThreadPool(&work_tp, &param_tp);
        assert(rc == true);

        param_tp.thread_number = 1;
        rc = initThreadPool(&write_tp, &param_tp);
        assert(rc == true);

        rc = initThreadPool(&read_tp, &param_tp);
        assert(rc == true);

        strcpy(param.host, "127.0.0.1");
        param.port = 10809;
        param.read_buffer_size = 1 << 12;
        param.read_tp = &read_tp;
        param.write_tp = &write_tp;
        param.worker_tp = &work_tp;
        param.request_sender = ClientRequestSenders;

        rc = initClient(&client, &param);
        assert(rc == true);

        int round;
        volatile int done_number = 0;
        int round_max = 1000000;
        for (round = 0; round < round_max; round++) {
                SamplePutRequest *sput = calloc(1, sizeof(*sput) + 1024);
                Sample *s1 = &sput->sample;
                strcpy(s1->bucket, "buck1");
                s1->id = round;
                s1->path = s1->data;
                sprintf(s1->path, "/var/log/message%dxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", round);
                s1->path_length = strlen(s1->path);
                s1->size = 1 << 30;

                Request *req = &sput->super;
                req->request_id = SampleRequestIdPut;
                req->resource_id = ResourceIdSample;

                bool free_req;

                rc = client.m->sendRequest(&client, &sput->super, UtClientSamplePutSendCallback, (void*)&done_number, &free_req);
                if (free_req) {
                        free(sput);
                }
                if (rc) {
//                        DLOG("Put success round:%d!!", round);
                } else {
                        __sync_add_and_fetch(&done_number, 1);
//                        DLOG("Send request failed!!");
                }
        }
        while (done_number != round_max) {
                sleep(1);
        }
        client.m->destroy(&client);
        read_tp.m->destroy(&read_tp);
        write_tp.m->destroy(&write_tp);
        work_tp.m->destroy(&work_tp);
        return 0;
}

