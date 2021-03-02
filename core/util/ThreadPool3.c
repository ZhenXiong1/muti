/*
 * thread_pool.c
 *
 *  Created on: Apr 12, 2019
 *      Author: rick
 */
#include <stdbool.h>

#include "util/ThreadPool.h"

static bool insertAndDoJob(ThreadPool *tp, Job *job) {
        (void)tp;
        job->doJob(job);
        return true;
}

static bool insertAndDoJobHash(ThreadPool *tp, Job *job, uint64_t hash) {
        (void)tp;(void)hash;
        job->doJob(job);
        return true;
}


static void destroy(ThreadPool *tp)
{
        (void)tp;
}

static ThreadPoolMethod m = {
	.insertTail = insertAndDoJob,
	.insertHead = insertAndDoJob,
        .insertTailHash = insertAndDoJobHash,
        .insertHeadHash = insertAndDoJobHash,
	.destroy = destroy,
};

bool initThreadPool3(ThreadPool *tp, ThreadPoolParam *param)
{
	tp->m = &m;
	tp->p = NULL;
	return true;
}
