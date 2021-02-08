/*
 * ErrorResponseEncoder.c
 *
 *  Created on: Jan 24, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <server/handler/RequestHandler.h>

bool ErrorResponseEncoder(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buff_len = sizeof(*resp);
        *buffer = (char *)resp;
        *free_resp = false;
        return true;
}

