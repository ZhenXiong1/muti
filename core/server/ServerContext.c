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

static void destroy(ServerContext* this) {
        this->sampleDao.m->destroy(&this->sampleDao);
}

static ServerContextMethod method = {
        .destroy = destroy,
};

bool initServerContext(ServerContext* this, ServerContextParam* param) {
        SampleDaoParam sd_param;
        int rc;

        this->p = NULL;
        this->m = &method;
        
        rc = initSampleDao(&this->sampleDao, &sd_param);
        if (rc == false) {
                goto out;
        }

out:
        return rc;
}


