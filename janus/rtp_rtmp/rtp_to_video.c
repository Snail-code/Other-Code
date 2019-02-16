#include "rtp_to_video.h"

//rtp视频上下文初始化
struct rtp_video_context_t*  rtp_video_decode_init(int video_payload_type, const char* video_payload_name, video_decode_packet_cb packet, void* cbdata)
{
	struct rtp_video_context_t* pvideo_context = NULL;
	int flag = 0;

	do
	{
		pvideo_context = (struct rtp_context_t*)malloc(sizeof(struct rtp_video_context_t));
		if (NULL == pvideo_context)
		{
			JANUS_LOG(LOG_ERR, "Init decode rtp video context filed, when malloc rtp_video_context_t err is %d.\n",errno);
			break;
		}
		pvideo_context->cbdata = cbdata;

		struct rtp_payload_t vidoe_hander_cb;
		vidoe_hander_cb.alloc = NULL;
		vidoe_hander_cb.free = NULL;
		vidoe_hander_cb.packet = packet;

		snprintf(pvideo_context->payload_name, sizeof(pvideo_context->payload_name), "%s", video_payload_name);

		pvideo_context->decoder = rtp_payload_decode_create(video_payload_type, video_payload_name, &vidoe_hander_cb, pvideo_context->cbdata);
		if (NULL == pvideo_context->decoder)
		{
			JANUS_LOG(LOG_ERR, "Init decode rtp video context filed when create decoder. err is %d.\n", errno);
			break;
		}
		
		pvideo_context->pData = (char*)malloc(1024 * 1024);
		pvideo_context->used_len = 0;
		pvideo_context->capacity = 1024 * 1024;

#ifdef TEST_DEBUG
		pvideo_context->fp = fopen("/tmp/dstfile.h264", "wb");
		if (pvideo_context->fp == NULL)
		{
			JANUS_LOG(LOG_ERR, "Init decode rtp video context filed, when open h264 file. err is %d.\n", errno);
			break;
		}
#endif // TEST_DEBUG

		flag = 1;
		JANUS_LOG(LOG_INFO, "Init decode rtp video context success \n");
	} while (0);

	if (!flag)
	{
		if (pvideo_context->decoder != NULL)
		{
			rtp_payload_decode_destroy(pvideo_context->decoder);
			pvideo_context->decoder = NULL;
		}
		
		if (pvideo_context != NULL)
		{
			free(pvideo_context);
			pvideo_context = NULL;
		}
	}
	return pvideo_context;
}

int rtp_video_decode_input(struct rtp_video_context_t* pContext,unsigned char* pbuf, int len)
{
	return rtp_payload_decode_input(pContext->decoder, pbuf, len);
}

void rtp_Video_decode_destory(struct rtp_video_context_t* pContext)
{
	if (pContext == NULL)
	{
		return;
	}
	if (pContext->decoder != NULL)
	{
		rtp_payload_decode_destroy(pContext->decoder);
		pContext->decoder = NULL;
	}

#ifdef TEST_DEBUG
	if (pContext->fp != NULL)
	{
		fclose(pContext->fp);
		pContext->fp = NULL;
	}
#endif // TEST_DEBUG

	free(pContext);
	pContext = NULL;
	JANUS_LOG(LOG_INFO, "RTP video decoder context destroy\n");
}

