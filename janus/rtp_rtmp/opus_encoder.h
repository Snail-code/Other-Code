#ifndef __OPUS_ENCODE_H__
#define __OPUS_ENCODE_H__
#include <stdio.h>
#include "libopus/include/opus/opus.h"
#include "libopus/include/opus/opus_multistream.h"
#include "libopus/include/opus/opusenc.h"
#include <stdint.h>
#include <assert.h>

#define IMIN(a,b) ((a) < (b) ? (a) : (b))   /**< Minimum int value.   */
#define IMAX(a,b) ((a) > (b) ? (a) : (b))   /**< Maximum int value.   */
#define PACKAGE_NAME "opus-tools"
#define PACKAGE_VERSION "1.1.1.1"
typedef void(*opus_encode_cb)(void* parame, unsigned char* pdata, int len, uint32_t timestamp);

#define INPUT_BUFFER_CHAR_SIZE 960*4
//#define INPUT_BUFFER_FLOAT_SIZE 960*4*2 //20msÊý¾Ý  48000/1000*20*sizeof(float)*channel

struct opus_encode_context_t
{
	char*        inputbuffer;
	uint16_t*    convert_buf;
	int          used_len;
	int          input_buf_capacity;
	opus_encode_cb cb;
	void*          param;
//	OggOpusEnc*    Enc;
//	OpusEncCallbacks EncCallbacks;
//	OggOpusComments*  comments;
//	opus_int32 rate;
	int channels;
	int family;
	uint32_t     timestamp;
//	FILE*        opus_fd;
	OpusEncoder* Enc;
};



struct opus_encode_context_t* pcm_to_opus_encode_init(int sampleRate, int channels, int family, opus_encode_cb cb, void* param);

int  pcm_to_opus_encode_input(struct opus_encode_context_t* pcontext, unsigned char* pdata, int len, uint32_t timestamp);

void pcm_to_opus_encode_destory(struct opus_encode_context_t* pcontext);

#endif // !__OPUS_ENCODE_H__

