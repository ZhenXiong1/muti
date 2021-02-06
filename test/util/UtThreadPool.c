/*
 * UtThreadPool.c
 *
 *  Created on: 2020\12\7
 *      Author: Rick
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util/ThreadPool.h"

pthread_mutex_t ut_lock;

typedef struct myJob {
        Job     super;
        int     num;
} MyJob;

void utJobDo(Job *job) {
        MyJob *myjob = (MyJob*)job;
        char msg[1024];
        int i;
//        pthread_mutex_lock(&ut_lock);
        for (i = 0; i < 1000; i++) {

                void *mm = malloc(1024);
                sprintf(msg, "Do job:%p num:%d\n", job, myjob->num);
                free(mm);

        }
//        pthread_mutex_unlock(&ut_lock);

}

int UtThreadPool(int argv, char **argvs) {
        ThreadPool tp;
        ThreadPoolParam param;
        int rc, i;

        (void)argv;(void)argvs;
        pthread_mutex_init(&ut_lock, NULL);

        param.do_batch = NULL;
        param.thread_number = 6;
        rc = initThreadPool(&tp, &param);
        if (rc == false) {
                printf("initThreadPool falied\n");
                exit(-1);
        }

        MyJob *job = malloc(sizeof(MyJob) * 10000);
        for (i = 0; i < 10000; i++) {
                job[i].super.doJob = utJobDo;
                job[i].num = i;
                tp.m->insertTail(&tp, &job[i].super);
        }
        tp.m->destroy(&tp);
        free(job);
	return 0;
}


