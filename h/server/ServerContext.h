/*
 * ServerContext.h
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#ifndef SERVER_SERVERCONTEXT_H_
#define SERVER_SERVERCONTEXT_H_
#include <stdbool.h>
#include <server/dao/SampleDao.h>

typedef struct ServerContext ServerContext;
typedef struct ServerContextMethod {
        void    (*destroy)(ServerContext*);
} ServerContextMethod;

struct ServerContext {
        void                    *p;
        ServerContextMethod     *m;
        SampleDao               sampleDao;
};

typedef struct ServerContextParam {

} ServerContextParam;

bool initServerContext(ServerContext*, ServerContextParam*);

#endif /* SERVER_SERVERCONTEXT_H_ */
