/*
 * RequestSenders.h
 *
 *  Created on: Feb 6, 2021
 *      Author: root
 */

#ifndef CLIENT_SENDER_REQUESTSENDERS_H_
#define CLIENT_SENDER_REQUESTSENDERS_H_
#include "sample/SampleRequestSender.h"
#include <share/Resources.h>

static RequestSender ClientRequestSenders[] = {
                {ResourceIdSample, SampleRequestEncoder, SampleResponseDecoder},
};

#endif /* CLIENT_SENDER_REQUESTSENDERS_H_ */
