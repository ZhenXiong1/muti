/*
 * thread_pool.c
 *
 *  Created on: Apr 12, 2019
 *      Author: rick
 */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "util/ThreadPool.h"
#include "util/LinkedList.h"

typedef enum ThreadPoolState {
	TP_STATE_INIT = 0,
	TP_STATE_START,
	TP_STATE_STOP
} ThreadPoolState;

typedef struct ThreadPoolPrivate {
	ThreadPoolState         state;
	pthread_t               pid;
	pthread_mutex_t         lck;
	pthread_cond_t          cond;
	DoJobBatch              do_batch;
	ListHead                work_head;
	ListHead                runing_head;
} ThreadPoolPrivate;

static bool insertTail(ThreadPool *tp, Job *job) {
	ThreadPoolPrivate *priv = tp->p;
	int rc = true;

	pthread_mutex_lock(&priv->lck);
	if (priv->state == TP_STATE_STOP) {
		rc = false;
	} else {
		listAddTail(&job->element, &priv->work_head);
		pthread_cond_signal(&priv->cond);
	}
	pthread_mutex_unlock(&priv->lck);
	return rc;
}

static bool insertTailHash(ThreadPool *tp, Job *job, uint64_t hash) {
        (void)hash;
        return insertTail(tp, job);
}

static bool insertHead(ThreadPool *tp, Job *job) {
	ThreadPoolPrivate *priv = tp->p;
	int rc = true;

	pthread_mutex_lock(&priv->lck);
	if (priv->state == TP_STATE_STOP) {
		rc = false;
	} else {
		listAdd(&job->element, &priv->work_head);
		pthread_cond_signal(&priv->cond);
	}
	pthread_mutex_unlock(&priv->lck);
	return rc;
}

static bool insertHeadHash(ThreadPool *tp, Job *job, uint64_t hash) {
        (void)hash;
        return insertHead(tp, job);
}

static void destroy(ThreadPool *tp)
{
	ThreadPoolPrivate *priv = tp->p;

	pthread_mutex_lock(&priv->lck);
	priv->state = TP_STATE_STOP;
	pthread_cond_signal(&priv->cond);
	pthread_mutex_unlock(&priv->lck);

	pthread_join(priv->pid, NULL);

	tp->p = NULL;
	free(priv);

}

static ThreadPoolMethod m = {
	.insertTail = insertTail,
	.insertTailHash = insertTailHash,
	.insertHead = insertHead,
	.insertHeadHash = insertHeadHash,
	.destroy = destroy,
};

static void __thread_pool_exec_one_by_one(struct list_head *head)
{
	Job *job, *job1;

	listForEachEntrySafe(job, job1, head, element) {
		listDel(&job->element);
		job->doJob(job);
	}
}

static void * threadPoolDoThread(void *p)
{
	ThreadPool *tp = p;
	ThreadPoolPrivate *priv = tp->p;

	pthread_mutex_lock(&priv->lck);
	priv->state = TP_STATE_START;
	while (1) {
		if (listEmpty(&priv->work_head)) {
			if (priv->state == TP_STATE_STOP) {
				break;
			}
			pthread_cond_wait(&priv->cond, &priv->lck);
		}
		listJoinTailInit(&priv->work_head, &priv->runing_head);
		pthread_mutex_unlock(&priv->lck);
		priv->do_batch(&priv->runing_head);
		pthread_mutex_lock(&priv->lck);
	}
	pthread_mutex_unlock(&priv->lck);
	return NULL;
}

bool initThreadPool1(ThreadPool *tp, ThreadPoolParam *param)
{
	ThreadPoolPrivate *priv;

	tp->m = &m;
	priv = calloc(sizeof(*priv), 1);
	tp->p = priv;

	priv->state = TP_STATE_INIT;
	listHeadInit(&priv->work_head);
	listHeadInit(&priv->runing_head);
	pthread_mutex_init(&priv->lck, NULL);
	pthread_cond_init(&priv->cond, NULL);
	if (param->do_batch){
		priv->do_batch = param->do_batch;
	} else {
		priv->do_batch = __thread_pool_exec_one_by_one;
	}
	pthread_create(&priv->pid, NULL, threadPoolDoThread, tp);

	while (priv->state == TP_STATE_INIT) {
		usleep(10000);
	}

	return true;
}
