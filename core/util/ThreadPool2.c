/*
 * thread_pool.c
 *
 *  Created on: Apr 12, 2019
 *      Author: rick
 */
#include <pthread.h>
#include <stdint.h>

#include "util/ThreadPool.h"

typedef struct ThreadPoolPrivate {
	ThreadPool	        *tp1;
	int			thread_num;
	volatile uint32_t	current_idx;
} ThreadPoolPrivate;

static bool insertTail(ThreadPool *tp, Job *job) {
	ThreadPoolPrivate *priv = tp->p;
	ThreadPool *tp1 = &priv->tp1[__sync_fetch_and_add(&priv->current_idx, 1)
					     % priv->thread_num];
	return tp1->m->insertTail(tp1, job);
}

static bool insertHead(ThreadPool *tp, Job *job) {
	ThreadPoolPrivate *priv = tp->p;
	ThreadPool *tp1 = &priv->tp1[__sync_fetch_and_add(&priv->current_idx, 1)
					     % priv->thread_num];
	return tp1->m->insertHead(tp1, job);
}

static bool insertHeadHash(ThreadPool *tp, Job *job, uint64_t hash) {
        ThreadPoolPrivate *priv = tp->p;
        ThreadPool *tp1 = &priv->tp1[hash % priv->thread_num];
        return tp1->m->insertHead(tp1, job);
}

static bool insertTailHash(ThreadPool *tp, Job *job, uint64_t hash) {
        ThreadPoolPrivate *priv = tp->p;
        ThreadPool *tp1 = &priv->tp1[hash % priv->thread_num];
        return tp1->m->insertTail(tp1, job);
}

static void destroy(ThreadPool *tp)
{
	ThreadPoolPrivate *priv = tp->p;
	int i;

	for (i = 0; i < priv->thread_num; i++) {
		ThreadPool *tp1 = &priv->tp1[i];
		tp1->m->destroy(tp1);
	}

	free(priv->tp1);
	free(priv);
}

static ThreadPoolMethod m = {
	.insertTail = insertTail,
	.insertHead = insertHead,
        .insertTailHash = insertTailHash,
        .insertHeadHash = insertHeadHash,
	.destroy = destroy,
};

extern int initThreadPool1(ThreadPool *tp, ThreadPoolParam *param);

bool initThreadPool2(ThreadPool *tp, ThreadPoolParam *param)
{
	ThreadPoolPrivate *priv;
	int i, tp_num, rc;

	tp->m = &m;
	priv = calloc(sizeof(*priv), 1);
	tp->p = priv;
	tp_num = param->thread_number;
	priv->thread_num = tp_num;
	param->thread_number = 1;
	priv->tp1 = malloc(sizeof(ThreadPool) * tp_num);

	for (i = 0; i < tp_num; i++) {
		rc = initThreadPool1(&priv->tp1[i], param);
		if (rc == false) break;
	}

	return rc;
}
