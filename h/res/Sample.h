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
        ListHead        *sample_list;
} SampleListResponse;

#endif /* RES_SAMPLE_H_ */
