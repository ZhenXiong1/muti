/*
 * log.h
 *
 *  Created on: Apr 24, 2014
 *      Author: Zhen_Xiong
 */

#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG
#	include <pthread.h>
#	if defined(WIN32) || defined(WIN64)
#		define ELOG(str, args...) \
			fprintf(stderr, "ERR: [%s %d TH:%p] "str"\n",  __FILE__, __LINE__, (void*)pthread_self().p, ##args); fflush(stderr);

#		define WLOG(str, args...) \
			printf("WARN: [%s %d TH:%p] "str"\n",  __FILE__, __LINE__, (void*)pthread_self().p, ##args); fflush(stdout);

#		define DLOG(str, args...) \
			printf("DEBUG: [%s %d TH:%p] "str"\n",  __FILE__, __LINE__, (void*)pthread_self().p, ##args); fflush(stdout);
#	else
#		define ELOG(str, args...) \
			fprintf(stderr, "ERR: [%s %d TH:%lu] "str"\n",  __FILE__, __LINE__, pthread_self(), ##args); fflush(stderr);

#		define WLOG(str, args...) \
			printf("WARN: [%s %d TH:%lu] "str"\n",  __FILE__, __LINE__, pthread_self(), ##args); fflush(stdout);

#		define DLOG(str, args...) \
			printf("DEBUG: [%s %d TH:%lu] "str"\n",  __FILE__, __LINE__, pthread_self(), ##args); fflush(stdout);
#	endif
#else
#	define ELOG(str, args...) \
		fprintf(stderr, "ERR: "str"\n",  ##args); fflush(stderr)

#	define WLOG(str, args...) \
		printf("WARN: "str"\n",  ##args); fflush(stdout)

#	define DLOG(str, args...) \
        printf("DEBUG: "str"\n",  ##args); fflush(stdout)
#endif

#endif /* LOG_H_ */
