/*
 * thread_pool.h
 *
 *  Created on: Apr 12, 2019
 *      Author: rick
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include "Object.h"
#include "util/LinkedList.h"

typedef struct Job Job;

typedef void (*DoJob)(Job *);
typedef void (*DoJobBatch)(ListHead*);

struct Job {
        ListElement	        element;
	DoJob			doJob;
};

typedef struct ThreadPool ThreadPool;
typedef struct ThreadPoolMethod {
        void    (*destroy)(ThreadPool*);
        bool (*insertTail)(ThreadPool*, Job *);
        bool (*insertHead)(ThreadPool*, Job *);
        bool (*insertTailHash)(ThreadPool*, Job *, uint64_t);
        bool (*insertHeadHash)(ThreadPool*, Job *, uint64_t);
} ThreadPoolMethod;

struct ThreadPool {
        void				*p;
        ThreadPoolMethod                *m;
};

typedef struct ThreadPoolParam {
	int		thread_number;
	DoJobBatch	do_batch;
} ThreadPoolParam;

bool initThreadPool(ThreadPool *, ThreadPoolParam*);

#endif /* TP_TP_IF_H_ */
