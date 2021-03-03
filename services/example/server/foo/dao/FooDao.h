/*
 * FooDao.h
 *
 *  Created on: Feb 2, 2021
 *      Author: root
 */

#ifndef SERVER_DAO_SAMPLEDAO_H_
#define SERVER_DAO_SAMPLEDAO_H_
#include <stdbool.h>
#include <example/share/Foo.h>

typedef struct FooDao FooDao;
typedef struct FooDaoMethod {
        Foo* (*getFoo)(FooDao*, int);
        int     (*putFoo)(FooDao*, Foo *foo);
        int     (*listFoo)(FooDao*, ListHead **head, int page, int page_size);
        void    (*destroy)(FooDao*);
} FooDaoMethod;

struct FooDao {
        void                 *p;
        FooDaoMethod      *m;
};

typedef struct FooDaoParam {

} FooDaoParam;

bool initFooDao(FooDao*, FooDaoParam*);

#endif /* SERVER_DAO_SAMPLEDAO_H_ */
