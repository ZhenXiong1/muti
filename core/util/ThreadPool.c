/*
 * thread_pool.c
 *
 *  Created on: Apr 12, 2019
 *      Author: rick
 */
#include <util/ThreadPool.h>

extern bool initThreadPool1(ThreadPool *tp, ThreadPoolParam *param);
extern bool initThreadPool2(ThreadPool *tp, ThreadPoolParam *param);
extern bool initThreadPool3(ThreadPool *tp, ThreadPoolParam *param);

bool initThreadPool(ThreadPool *tp, ThreadPoolParam *param)
{
	switch(param->thread_number) {
	case 1:
		return initThreadPool1(tp, param);
	case 0:
	        return initThreadPool3(tp, param);
	default:
		return initThreadPool2(tp, param);
	}
}
