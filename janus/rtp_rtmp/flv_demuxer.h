#ifndef __FLV_DEMUXER__H__
#define __FLV_DEMUXER__H__
#include "flv-demuxer.h"
#include "flv-reader.h"
#include "flv-proto.h"
#include <assert.h>
#include <stdio.h>
#include "debug.h"
typedef void(*flv_demuxer_video_audio_cb)(void* param, int codec, const unsigned char* data, size_t bytes, uint32_t pts, uint32_t dts, int flags);


struct flv_demuxer_video_audio_context_t
{
	flv_demuxer_t* flv;
	void *         param;
	flv_demuxer_video_audio_cb cb;

	FILE*          video_fd;
	FILE*          audio_fd;
};


struct flv_demuxer_video_audio_context_t * flv_demuxer_video_audio_init(void* parma, flv_demuxer_video_audio_cb cbfun);

int flv_demuxer_video_audio_input(struct flv_demuxer_video_audio_context_t* demuxer, int type, const void* data, size_t bytes, uint32_t timestamp);

void flv_demuxer_video_audio_destory(struct flv_demuxer_video_audio_context_t *pcontext);




#endif // !__FLV_DEMUXER__H__

