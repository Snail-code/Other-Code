#ifndef __AAC_ENCODEC_H__
#define __AAC_ENCODEC_H__

#include "fdk-aac/include/fdk-aac/aacenc_lib.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "debug.h"
typedef void(*pcm_encode_cb)(void* parame, unsigned char* pdata, int len, uint32_t timestamp);

struct aac_encode_context_t
{
	int bitrate;
	int ch;
	int format;
	int sample_rate; 
	int channels; 
	int bits_per_sample;
	int input_size;
	uint8_t* input_buf;
	int16_t* convert_buf;
	int aot;
	int afterburner ;
	int eld_sbr;
	int vbr;
	HANDLE_AACENCODER handle;
	CHANNEL_MODE mode;
	AACENC_InfoStruct info;
	pcm_encode_cb cbfun;
	void*         cbdata;
	uint32_t      timestamp;
	int           usedlen;

#ifdef TEST_DEBUG
	FILE*         fd;
#endif // TEST_DEBUG

};

struct  aac_encode_context_t* aac_encoder_init(int channle, int sample_rate, int format, int aot, int vbr, pcm_encode_cb cbfun,void* cbdata);
int aac_encode_input(struct  aac_encode_context_t* pcontext,const unsigned char* pdata, int len, uint32_t timestamp);
void aac_encode_destory(struct  aac_encode_context_t* pcontext);

#endif __AAC_ENCODEC_H__ 
