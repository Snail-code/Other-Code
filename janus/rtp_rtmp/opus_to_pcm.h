#ifndef __OPUS_TO_PCM__H
#define __OPUS_TO_PCM__H
#include "libopus/include/opus/opus.h"
#include <stdio.h>
#include "debug.h"

typedef void(*opus_to_pcm_cb)(void* parame, unsigned char* pdata, int len, uint32_t timestamp);

struct opus_to_pcm_context_t
{
	OpusDecoder*   decoder;
	opus_to_pcm_cb cb;
	void*          cbdata;
	int            src_seq;
	int			   max_frame_size;
	int			   channels;
	short*		   out;

#ifdef TEST_DEBUG
	FILE*          fd;
#endif
};

struct opus_to_pcm_context_t* opus_to_pcm_init(int sampleRate, int channels, opus_to_pcm_cb cb,void* param);

int  opus_to_pcm_decode(struct opus_to_pcm_context_t* pcontext,unsigned char* pdata,int len,uint32_t timestamp,int seq);

void opus_to_pcm_destory(struct opus_to_pcm_context_t* pcontext);

#endif //__OPUS_TO_PCM__H