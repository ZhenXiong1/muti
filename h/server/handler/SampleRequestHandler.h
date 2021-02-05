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

extern void SampleActionGet(RequestW *);
extern void SampleActionPut(RequestW *);
extern void SampleActionList(RequestW *);

extern Action SampleActions[];

extern Request* SampleRequestDecoderCGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Request* SampleRequestDecoderCPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Request* SampleRequestDecoderCList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);

extern RequestDecoder* SampleRequestDecoder[];

bool SampleResponseEncoderCGet(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleResponseEncoderCPut(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleResponseEncoderCList(Response *resp, char **buffer, size_t *buff_len, bool *free_resp);

extern ResponseEncoder* SampleResponseEncoder[];

#endif /* RESSAMPLE_H_ */
