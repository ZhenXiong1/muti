/*
 * ServerRequestHandlers.h
 *
 *  Created on: Jan 8, 2021
 *      Author: rick
 */

#ifndef ServerRequestHandlers_H_
#define ServerRequestHandlers_H_

#include <server/handler/SampleRequestHandler.h>

static RequestHandler ServerRequestHandlers[] = {
                {SampleActions, SampleRequestDecoder, SampleResponseEncoder},
};


#endif /* ServerRequestHandlers_H_ */
