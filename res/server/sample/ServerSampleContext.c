/*
 * ServerSampleContext.c
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#include "ServerSampleContext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void destroy(ServerSampleContext* this) {
        this->sampleDao.m->destroy(&this->sampleDao);
}

static ServerSampleContextMethod method = {
        .destroy = destroy,
};

bool initServerSampleContext(ServerSampleContext* this, ServerSampleContextParam* param) {
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


