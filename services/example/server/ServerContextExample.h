/*
 * ServerFooContext.h
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#ifndef SERVER_SERVERSAMPLECONTEXT_H_
#define SERVER_SERVERSAMPLECONTEXT_H_
#include <stdbool.h>

#include "foo/dao/FooDao.h"

typedef struct ServerContextExample ServerContextExample;
typedef struct ServerContextExampleMethod {
        void    (*destroy)(ServerContextExample*);
} ServerContextExampleMethod;

struct ServerContextExample {
        void                    *p;
        ServerContextExampleMethod     *m;
        FooDao               fooDao;
};

typedef struct ServerContextExampleParam {

} ServerContextExampleParam;

bool initServerFooContext(ServerContextExample*, ServerContextExampleParam*);

#endif /* SERVER_SERVERSAMPLECONTEXT_H_ */
