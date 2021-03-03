/*
 * Foo.h
 *
 *  Created on: Jan 30, 2021
 *      Author: root
 */

#ifndef RES_SAMPLE_H_
#define RES_SAMPLE_H_

#include <res/Resource.h>
#include <util/LinkedList.h>

typedef enum {
        FooRequestIdGet = 0,
        FooRequestIdPut = 1,
        FooRequestIdList = 2,
} FooRequestId;

typedef struct Foo {
        ListElement     element;
        int             id;
        char            bucket[64];
        size_t          path_length;
        char            *path;
        size_t          size;
        char            data[];
} Foo;

typedef struct FooGetRequest {
        Request         super;
        int             id;
} FooGetRequest;

typedef struct FooGetResponse {
        Response        super;
        Foo          foo;
} FooGetResponse;

typedef struct FooPutRequest {
        Request         super;
        Foo          foo;
} FooPutRequest;

typedef struct FooPutResponse {
        Response        super;
} FooPutResponse;

typedef struct FooListRequest {
        Request         super;
        int32_t         page;
        int32_t         page_size;
} FooListRequest;

typedef struct FooListResponse {
        Response        super;
        uint32_t        length;
        union {
        	ListHead	foo_head;	// For decoder
        	ListHead        *foo_list;	// For encoder
        };
} FooListResponse;

#endif /* RES_SAMPLE_H_ */
