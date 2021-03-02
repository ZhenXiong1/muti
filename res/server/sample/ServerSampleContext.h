/*
 * ServerSampleContext.h
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#ifndef SERVER_SERVERSAMPLECONTEXT_H_
#define SERVER_SERVERSAMPLECONTEXT_H_
#include <stdbool.h>
#include "dao/SampleDao.h"

typedef struct ServerSampleContext ServerSampleContext;
typedef struct ServerSampleContextMethod {
        void    (*destroy)(ServerSampleContext*);
} ServerSampleContextMethod;

struct ServerSampleContext {
        void                    *p;
        ServerSampleContextMethod     *m;
        SampleDao               sampleDao;
};

typedef struct ServerSampleContextParam {

} ServerSampleContextParam;

bool initServerSampleContext(ServerSampleContext*, ServerSampleContextParam*);

#endif /* SERVER_SERVERSAMPLECONTEXT_H_ */
