/*
 * Resource.h
 *
 *  Created on: Jan 30, 2021
 *      Author: root
 */

#ifndef RES_RESOURCE_H_
#define RES_RESOURCE_H_

#include <stdint.h>

typedef enum RRType {
        RRTYPE_C = 0,
        RRTYPE_JAVA,
        RRTYPE_HTTP,
        RRTYPE_MAX
} RRType;

typedef struct Response {
        uint8_t         response_type; //RRType
        int8_t          error_id;
} Response;

typedef struct Request {
        uint8_t         request_type; //RRType
        uint8_t         resource_id;
        uint16_t        request_id;
} Request;

#endif /* RES_RESOURCE_H_ */
