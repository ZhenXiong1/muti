/*
 * SampleRequestSender.h
 *
 *  Created on: Feb 6, 2021
 *      Author: Zhen Xiong
 */

#ifndef CLIENT_SENDER_SAMPLEREQUESTSENDER_H_
#define CLIENT_SENDER_SAMPLEREQUESTSENDER_H_

#include <client/sender/RequestSender.h>

extern Response* SampleResponseDecoderCGet(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Response* SampleResponseDecoderCPut(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);
extern Response* SampleResponseDecoderCList(char *buffer, size_t buff_len, size_t *consume_len, bool *free_req);

extern ResponseDecoder SampleResponseDecoder[];

bool SampleRequestEncoderCGet(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleRequestEncoderCPut(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);
bool SampleRequestEncoderCList(Request *resp, char **buffer, size_t *buff_len, bool *free_resp);

extern RequestEncoder SampleRequestEncoder[];

#endif /* CLIENT_SENDER_SAMPLEREQUESTSENDER_H_ */
