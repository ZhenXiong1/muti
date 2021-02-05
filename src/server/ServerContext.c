/*
 * ServerContext.c
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <server/ServerContext.h>

static void destroy(ServerContext* obj) {
        obj->sampleDao.m->destroy(&obj->sampleDao);
}

static ServerContextMethod method = {
        .destroy = destroy,
};

bool initServerContext(ServerContext* obj, ServerContextParam* param) {
        SampleDaoParam sd_param;
        int rc;

        obj->p = NULL;
        obj->m = &method;
        
        rc = initSampleDao(&obj->sampleDao, &sd_param);
        if (rc == false) {
                goto out;
        }

out:
        return rc;
}


