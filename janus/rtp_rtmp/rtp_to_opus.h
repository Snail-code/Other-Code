#ifndef __RTP_TO_opus__
#define __RTP_TO_opus__

#include <stdint.h>
#include "rtp-profile.h"
#include "rtp-payload.h"
#include "librtp/include/rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opus.h"
#include <ogg/ogg.h>
#include "debug.h"

//rtp解封装回调函数
typedef void(*opus_decode_packet_cb) (void* param, const void *packet, int bytes, uint32_t timestamp, int seq);


struct rtp_opus_context_t
{

#ifdef TEST_DEBUG
	FILE* fp;
#endif // !TEST_DEBUG
	opus_decode_packet_cb   cbfun;
	void*					cbdata; 
	int						seq;
	int                     payload_type;
	uint32_t				timestamp;
	ogg_stream_state *		stream;
	ogg_int64_t				granulepos;
	int                     channle;
	int                     sample;
};

//rtp视频解封装上下文初始化
struct rtp_opus_context_t*  rtp_opus_decode_init(int opus_payload_type, const char* opus_payload_name, opus_decode_packet_cb packet, void* cbdata, int opus_sample, int channel);

//rtp负载解析输入
int rtp_opus_decode_input(struct rtp_opus_context_t* pContext, unsigned char* pbuf, int len);

//rtp视频解封装上下文销毁
void rtp_opus_decode_destory(struct rtp_opus_context_t* pContext);


//rtp视频封装回调函数
typedef void(*opus_encode_packet_cb)(void* param, const void *packet, int bytes, uint32_t timestamp, int flags);

//rtp视频封装上下文初始化
struct rtp_opus_context_t*  rtp_opus_encode_init(int opus_payload_type, const char* opus_payload_name, opus_encode_packet_cb packet_cb, void* cbdata, int opus_sample, int channel);

//rtp视频封装输入
int rtp_opus_encode_input(struct rtp_opus_context_t* pContext, unsigned char* pbuf, int len, uint32_t timestamp);

//rtp视频封装上下文销毁
void rtp_opus_encode_destory(struct rtp_opus_context_t* pContext);

#endif // !__RTP_TO_opus__
