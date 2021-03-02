/*
 * SampleDao.h
 *
 *  Created on: Feb 2, 2021
 *      Author: root
 */

#ifndef SERVER_DAO_SAMPLEDAO_H_
#define SERVER_DAO_SAMPLEDAO_H_
#include <share/Sample.h>
#include <stdbool.h>

typedef struct SampleDao SampleDao;
typedef struct SampleDaoMethod {
        Sample* (*getSample)(SampleDao*, int);
        int     (*putSample)(SampleDao*, Sample *sample);
        int     (*listSample)(SampleDao*, ListHead **head, int page, int page_size);
        void    (*destroy)(SampleDao*);
} SampleDaoMethod;

struct SampleDao {
        void                 *p;
        SampleDaoMethod      *m;
};

typedef struct SampleDaoParam {

} SampleDaoParam;

bool initSampleDao(SampleDao*, SampleDaoParam*);

#endif /* SERVER_DAO_SAMPLEDAO_H_ */
