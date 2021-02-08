/*
 * ErrorResponseDecoder.c
 *
 *  Created on: Feb 8, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <client/sender/RequestSender.h>

Response* ErrorResponseDecoder(char *buffer, size_t buff_len, size_t *consume_len, bool *free_resp) {
        size_t len = sizeof(Response);
        if (buff_len < len) return NULL;
        *consume_len = len;
        *free_resp = false;
        return (Response*) buffer;
}


