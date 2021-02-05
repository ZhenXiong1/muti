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

#include <server/handler/ErrorResponseEncoder.h>

bool ErrorResponseEncoderC(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buff_len = sizeof(*resp);
        *buffer = (char *)resp;
        *free_resp = false;
        return true;
}

bool ErrorResponseEncoderJava(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        *buff_len = sizeof(*resp);
        *buffer = (char *)resp;
        *free_resp = false;
        return true;
}

bool ErrorResponseEncoderHttp(Response *resp, char **buffer, size_t *buff_len, bool *free_resp) {
        (void)resp; (void)buffer; (void)buff_len; (void)free_resp;
        assert(0);
        return false;
}

ResponseEncoder errorResponseEncoder[] = {
                ErrorResponseEncoderC,
                ErrorResponseEncoderJava,
                ErrorResponseEncoderHttp
};

