/*
 * SampleRequestSender.h
 *
 *  Created on: Feb 6, 2021
 *      Author: Zhen Xiong
 */

#ifndef CLIENT_SENDER_SAMPLEREQUESTSENDER_H_
#define CLIENT_SENDER_SAMPLEREQUESTSENDER_H_

#include <client/RequestSender.h>

extern Response* SampleResponseDecoderGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Response* SampleResponseDecoderPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Response* SampleResponseDecoderList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);

extern ResponseDecoder SampleResponseDecoder[];

bool SampleRequestEncoderGet(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleRequestEncoderPut(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleRequestEncoderList(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);

extern RequestEncoder SampleRequestEncoder[];

#endif /* CLIENT_SENDER_SAMPLEREQUESTSENDER_H_ */
