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
#include <client/sender/RequestSenders.h>
#include <res/Sample.h>

void UtClientSamplePutSendCallback(Client *client, Response *resp, void *arg) {
        assert(resp->error_id == 0);
        sem_t *sem = arg;
        sem_post(sem);
}

int UtClient(int argv, char **argvs) {
        Client client;
        ClientParam param;
        ThreadPool work_tp, write_tp, read_tp;
        ThreadPoolParam param_tp;

        param_tp.do_batch = NULL;
        param_tp.thread_number = 6;

        bool rc = initThreadPool(&work_tp, &param_tp);
        assert(rc == true);

        rc = initThreadPool(&write_tp, &param_tp);
        assert(rc == true);

        rc = initThreadPool(&read_tp, &param_tp);
        assert(rc == true);

        strcpy(param.host, "127.0.0.1");
        param.port = 10809;
        param.read_buffer_size = 1 << 20;
        param.read_tp = &read_tp;
        param.write_tp = &write_tp;
        param.worker_tp = &work_tp;
        param.request_sender = ClientRequestSenders;

        rc = initClient(&client, &param);
        assert(rc == true);

        SamplePutRequest *sput = malloc(sizeof(*sput) + 1024);
        Sample *s1 = &sput->sample;
        strcpy(s1->bucket, "buck1");
        s1->id = 1000;
        s1->path = s1->data;
        strcpy(s1->path, "/var/log/message");
        s1->path_length = strlen(s1->path);
        s1->size = 1 << 30;

        Request *req = &sput->super;
        req->request_id = SampleRequestIdPut;
        req->resource_id = ResourceIdSample;

        sem_t sem;
        sem_init(&sem, 0, 0);
        bool free_req;

        client.m->sendRequest(&client, &sput->super, UtClientSamplePutSendCallback, &sem, &free_req);
        if (free_req) {
                free(sput);
        }
        sem_wait(&sem);
        DLOG("Put success!!");
        client.m->destroy(&client);
        read_tp.m->destroy(&read_tp);
        write_tp.m->destroy(&write_tp);
        work_tp.m->destroy(&work_tp);
        return 0;
}

