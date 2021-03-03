/*
 * ServerRequestHandlers.h
 *
 *  Created on: Jan 8, 2021
 *      Author: rick
 */

#ifndef ServerRequestHandlers_H_
#define ServerRequestHandlers_H_

#include <example/server/foo/FooHandler.h>
#include "../share/ExampleResources.h"

static RequestHandler ExampleHandlers[] = {
                {ResourceIdFoo, FooActions, FooRequestDecoder, FooResponseEncoder},
};


#endif /* ServerRequestHandlers_H_ */
