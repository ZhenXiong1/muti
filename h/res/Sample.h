/*
 * Sample.h
 *
 *  Created on: Jan 30, 2021
 *      Author: root
 */

#ifndef RES_SAMPLE_H_
#define RES_SAMPLE_H_

#include "Resource.h"
#include <util/LinkedList.h>

typedef struct Sample {
        ListElement     element;
        int             id;
        char            bucket[64];
        size_t          path_length;
        char            *path;
        size_t          size;
        char            data[];
} Sample;

typedef enum {
        SampleRequestIdGet = 0,
        SampleRequestIdPut = 1,
        SampleRequestIdList = 2,
} SampleRequestId;

typedef struct SampleGetRequest {
        Request         super;
        int             id;
} SampleGetRequest;

typedef struct SampleGetResponse {
        Response        super;
        Sample          sample;
} SampleGetResponse;

typedef struct SamplePutRequest {
        Request         super;
        Sample          sample;
} SamplePutRequest;

typedef struct SamplePutResponse {
        Response        super;
} SamplePutResponse;

typedef struct SampleListRequest {
        Request         super;
        int32_t         page;
        int32_t         page_size;
} SampleListRequest;

typedef struct SampleListResponse {
        Response        super;
        uint32_t        length;
        union {
        	ListHead	sample_head;	// For decoder
        	ListHead        *sample_list;	// For encoder
        };
} SampleListResponse;

#endif /* RES_SAMPLE_H_ */
