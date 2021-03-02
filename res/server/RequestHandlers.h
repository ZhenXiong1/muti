/*
 * ServerRequestHandlers.h
 *
 *  Created on: Jan 8, 2021
 *      Author: rick
 */

#ifndef ServerRequestHandlers_H_
#define ServerRequestHandlers_H_

#include "sample/SampleRequestHandler.h"
#include <share/Resources.h>

static RequestHandler ServerRequestHandlers[] = {
                {ResourceIdSample, SampleActions, SampleRequestDecoder, SampleResponseEncoder},
};


#endif /* ServerRequestHandlers_H_ */
