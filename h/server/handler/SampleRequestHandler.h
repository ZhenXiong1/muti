/*
 * Sample.h
 *
 *  Created on: Jan 8, 2021
 *      Author: rick
 */

#ifndef RESSAMPLE_H_
#define RESSAMPLE_H_
#include <server/handler/RequestHandler.h>
#include <stddef.h>

extern void SampleActionGet(SRequest *);
extern void SampleActionPut(SRequest *);
extern void SampleActionList(SRequest *);

extern Action SampleActions[];

extern Request* SampleRequestDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Request* SampleRequestDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Request* SampleRequestDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);

extern RequestDecoder SampleRequestDecoder[];

bool SampleResponseEncoderGet(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleResponseEncoderPut(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleResponseEncoderList(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);

extern ResponseEncoder SampleResponseEncoder[];

#endif /* RESSAMPLE_H_ */
