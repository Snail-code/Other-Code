#ifndef __AAC_DECODER_H__
#define __AAC_DECODER_H__
#include <stdio.h>
#include "fdk-aac/include/fdk-aac/aacdecoder_lib.h"
#include <stdint.h>
#include "debug.h"


#define  DECODE_PCM_BUF_LEN 50*1024

typedef void(*aac_decoder_cb)(void* param, const unsigned char* pdata, int bytes, uint32_t timestamp);

struct adts_header_t
{
	unsigned char syncword_0_to_8 : 8;
	unsigned char protection_absent : 1;
	unsigned char layer : 2;
	unsigned char ID : 1;
	unsigned char syncword_9_to_12 : 4;

	unsigned char channel_configuration_0_bit : 1;
	unsigned char private_bit : 1;
	unsigned char sampling_frequency_index : 4;
	unsigned char profile : 2;

	unsigned char frame_length_0_to_1 : 2;
	unsigned char copyrignt_identification_start : 1;
	unsigned char copyright_identification_bit : 1;
	unsigned char home : 1;
	unsigned char original_or_copy : 1;
	unsigned char channel_configuration_1_to_2 : 2;

	unsigned char frame_length_2_to_9 : 8;

	unsigned char adts_buffer_fullness_0_to_4 : 5;
	unsigned char frame_length_10_to_12 : 3;

	unsigned char number_of_raw_data_blocks_in_frame : 2;
	unsigned char adts_buffer_fullness_5_to_10 : 6;
};

struct aac_decoder_context_t
{
	void*             param;
	aac_decoder_cb   cbfun;
	HANDLE_AACDECODER dec;
	int               is_adts;
	CStreamInfo *     info;
	int               sample_bits;
	unsigned int      filled_bytes;
	int               channles;
	int               sample;
	char*             pcm_buf;
	int               pcm_buf_len;
	int				  n;
	FILE*             pcm_fd;
};

struct aac_decoder_context_t* aac_decoder_init(int channle, int sample, void* param, aac_decoder_cb cb);
int aac_decoder_input(struct aac_decoder_context_t* pcontext, const char* pdata, int bytes,int timestamp);
void aac_decoder_destroy(struct aac_decoder_context_t* pcontext);

#endif // !__AAC_DECODER_H__

