/*
 * RequestSenders.h
 *
 *  Created on: Feb 6, 2021
 *      Author: root
 */

#ifndef CLIENT_SENDER_REQUESTSENDERS_H_
#define CLIENT_SENDER_REQUESTSENDERS_H_
#include <client/sender/SampleRequestSender.h>

static RequestSender ClientRequestSenders[] = {
                {SampleRequestEncoder, SampleResponseDecoder},
};

#endif /* CLIENT_SENDER_REQUESTSENDERS_H_ */
