/*
 * Resource.h
 *
 *  Created on: Jan 30, 2021
 *      Author: root
 */

#ifndef RES_RESOURCE_H_
#define RES_RESOURCE_H_

#include <stdint.h>

typedef struct Response {
        int8_t          error_id;
        uint32_t        sequence;
} Response;

typedef struct Request {
        uint16_t        resource_id;
        uint16_t        request_id;
        uint32_t        sequence;
} Request;

#endif /* RES_RESOURCE_H_ */
