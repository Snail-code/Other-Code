#ifndef __RTP_TO_VIDEO__
#define __RTP_TO_VIDEO__  

#include <stdint.h>
#include "rtp-profile.h"
#include "rtp-payload.h"
#include "librtp/include/rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"

struct rtp_video_context_t
{
#ifdef TEST_DEBUG
	FILE* fp;
#endif // TEST_DEBUG
	char  payload_name[64];
	void* decoder;
	void* encoder;
	void* cbdata;
	int   payload_type;
	char* pData;
	int   used_len;
	int   capacity;
};

//rtp解封装回调函数(回调的h264数据不带 00 00 00 01或者 00 00 01)
typedef void (*video_decode_packet_cb) (void* param, const void *packet, int bytes, uint32_t timestamp, int flags) ;

//rtp视频解封装上下文初始化
struct rtp_video_context_t*  rtp_video_decode_init(int video_payload_type, const char* video_payload_name, video_decode_packet_cb packet, void* cbdata);

//rtp负载解析输入
int rtp_video_decode_input(struct rtp_video_context_t* pContext,unsigned char* pbuf,int len);

//rtp视频解封装上下文销毁
void rtp_Video_decode_destory(struct rtp_video_context_t* pContext);


#endif // !__RTP_TO_VIDEO__
