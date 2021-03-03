/*
 * RequestSenders.h
 *
 *  Created on: Feb 6, 2021
 *      Author: root
 */

#ifndef CLIENT_SENDER_REQUESTSENDERS_H_
#define CLIENT_SENDER_REQUESTSENDERS_H_
#include <example/client/foo/FooSender.h>
#include <example/share/ExampleResources.h>

static RequestSender ExampleSenders[] = {
                {ResourceIdFoo, FooRequestEncoder, FooResponseDecoder},
};

#endif /* CLIENT_SENDER_REQUESTSENDERS_H_ */
