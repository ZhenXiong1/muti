/*
 * ServerFooContext.c
 *
 *  Created on: Feb 3, 2021
 *      Author: root
 */

#include "ServerContextExample.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void destroy(ServerContextExample* this) {
        this->fooDao.m->destroy(&this->fooDao);
}

static ServerContextExampleMethod method = {
        .destroy = destroy,
};

bool initServerFooContext(ServerContextExample* this, ServerContextExampleParam* param) {
        FooDaoParam sd_param;
        int rc;

        this->p = NULL;
        this->m = &method;
        
        rc = initFooDao(&this->fooDao, &sd_param);
        if (rc == false) {
                goto out;
        }

out:
        return rc;
}


