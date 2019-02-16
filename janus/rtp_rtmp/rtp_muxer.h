#ifndef __RTP_MUXER_H__
#define __RTP_MUXER_H__
#include <stdio.h>
#include <stdint.h>
#include "librtp/include/rtp-profile.h"
#include "librtp/include/rtp-payload.h"
#include "librtp/include/rtp.h"
#include "debug.h"

typedef enum 
{
	RTP_VIDEO,
	RTP_AUDIO
}RTP_TYPE;

typedef void(*rtp_muxer_packet_cb) (void* param, const void *packet, int bytes, RTP_TYPE type);

#define MUXER_BUFFER_LEN 4096*2;
struct rtp_muxer_context_t
{
	rtp_muxer_packet_cb video_cbfun;
	char  video_payload_name[64];
	void* video_muxer;
	int   video_payload_type;
	void* video_param;
	uint32_t video_ssrc;
	rtp_muxer_packet_cb audio_cbfun;
	char  audio_payload_name[64];
	void* audio_muxer;
	int   audio_payload_type;
	void* audio_param;
	uint32_t audio_ssrc;
	char* muxer_buffer;
	int   muxer_buffer_len;
	uint32_t    audio_timestamp;
	uint32_t    video_timestamp;
	FILE*     rtp_fd;
};

struct rtp_muxer_context_t*  rtp_muxer_init(
	int video_payload_type,
	const char* video_payload_name,
	rtp_muxer_packet_cb video_cbfun,
	void* vidoe_cbdata,
	int audio_payload_type,
	const char* audio_payload_name,
	rtp_muxer_packet_cb audio_cbfun,
	void* audio_cbdata);


int rtp_muxer_input(struct rtp_muxer_context_t* pContext, unsigned char* pbuf, int len,uint32_t timestamp, RTP_TYPE type);


void rtp_muxer_destory(struct rtp_muxer_context_t* pContext);


#endif // !__RTP_MUXER_H__

