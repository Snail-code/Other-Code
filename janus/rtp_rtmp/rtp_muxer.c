#include "rtp_muxer.h"
#include "md5_tool/md5_tool.h"

static void video_packet (void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	struct rtp_muxer_context_t* pmuxer_context = (struct rtp_muxer_context_t*)param;
	pmuxer_context->video_cbfun(pmuxer_context, packet, bytes, RTP_VIDEO);
}
static void audio_packet(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	struct rtp_muxer_context_t* pmuxer_context = (struct rtp_muxer_context_t*)param;
	pmuxer_context->audio_cbfun(pmuxer_context, packet, bytes, RTP_AUDIO);
}
static void* alloc_packet(void* param, int bytes)
{
	struct rtp_muxer_context_t* pmuxer_context = (struct rtp_muxer_context_t*)param;
    if (bytes > pmuxer_context->muxer_buffer_len)
    {
		pmuxer_context->muxer_buffer = (char*)realloc(pmuxer_context->muxer_buffer, bytes + 1024);
		pmuxer_context->muxer_buffer_len = bytes + 1024;
    }
	return pmuxer_context->muxer_buffer;
}
static void* free_packet(void* param, void *packet)
{
	//struct rtp_muxer_context_t* pmuxer_context = (struct rtp_muxer_context_t*)param;
}


struct rtp_muxer_context_t* rtp_muxer_init(int video_payload_type, 
	const char* video_payload_name, 
	rtp_muxer_packet_cb video_cbfun, 
	void* video_cbdata,
	int audio_payload_type,
	const char* audio_payload_name,
	rtp_muxer_packet_cb audio_cbfun,
	void* audio_cbdata)
{
	struct rtp_muxer_context_t* pmuxer_context = NULL;
	int flag = 0;
	do
	{
		pmuxer_context = (struct rtp_context_t*)malloc(sizeof(struct rtp_muxer_context_t));
		if (NULL == pmuxer_context)
		{
			JANUS_LOG(LOG_ERR, "Init encode rtp video context filed, when malloc rtp_video_context_t err is %d.\n", errno);
			break;
		}
		pmuxer_context->video_param = video_cbdata;
		pmuxer_context->video_cbfun = video_cbfun;
		pmuxer_context->video_payload_type = video_payload_type;
		struct rtp_payload_t video_hander_cb;
		video_hander_cb.alloc = alloc_packet;
		video_hander_cb.free = free_packet;
		video_hander_cb.packet = video_packet;
		snprintf(pmuxer_context->video_payload_name, sizeof(pmuxer_context->video_payload_name), "%s", video_payload_name);
		pmuxer_context->video_ssrc = random32(pmuxer_context->video_payload_type);
		pmuxer_context->video_muxer = rtp_payload_encode_create(
			video_payload_type,
			video_payload_name,
			0,
			pmuxer_context->video_ssrc, 
			&video_hander_cb,
			pmuxer_context);
		if (NULL == pmuxer_context->video_muxer)
		{
			JANUS_LOG(LOG_ERR, "Init encode rtp video context filed when create encoder. err is %d.\n", errno);
			break;
		}

		pmuxer_context->audio_param = audio_cbdata;
		pmuxer_context->audio_cbfun = audio_cbfun;
		pmuxer_context->audio_payload_type = audio_payload_type;
		pmuxer_context->audio_timestamp = 0;
		struct rtp_payload_t audio_hander_cb;
		audio_hander_cb.alloc = alloc_packet;
		audio_hander_cb.free = free_packet;
		audio_hander_cb.packet = audio_packet;
		snprintf(pmuxer_context->audio_payload_name, sizeof(pmuxer_context->audio_payload_name), "%s", audio_payload_name);
		pmuxer_context->audio_ssrc = random32(pmuxer_context->audio_payload_type);
		pmuxer_context->audio_muxer = rtp_payload_encode_create(
			audio_payload_type,
			audio_payload_name,
			0,
			pmuxer_context->audio_ssrc,
			&audio_hander_cb,
			pmuxer_context);

		if (NULL == pmuxer_context->audio_muxer)
		{
			JANUS_LOG(LOG_ERR, "Init encode rtp audio context filed when create encoder. err is %d.\n", errno);
			break;
		}
		pmuxer_context->muxer_buffer_len = MUXER_BUFFER_LEN;
		pmuxer_context->muxer_buffer = (char*)malloc(pmuxer_context->muxer_buffer_len);
		if (pmuxer_context->muxer_buffer ==NULL)
		{
			JANUS_LOG(LOG_ERR, "Init encode rtp audio context filed, when malloc buffer. err is %d.\n", errno);
			break;
		}
		pmuxer_context->rtp_fd = fopen("/tmp/rtp.rtp", "wb");
		flag = 1;
		JANUS_LOG(LOG_INFO, "Init encode rtp audio context success \n");
	} while (0);

	if (!flag)
	{
		rtp_muxer_destory(pmuxer_context);
	}
	return pmuxer_context;
}

int rtp_muxer_input(struct rtp_muxer_context_t* pContext, unsigned char* pbuf, int len, uint32_t timestamp, RTP_TYPE type)
{
	
	if (type == RTP_AUDIO)
	{
		pContext->audio_timestamp = timestamp;
	}
	else
	{
		pContext->video_timestamp = timestamp;
	}
	rtp_payload_encode_input(type == RTP_AUDIO ? pContext->audio_muxer : pContext->video_muxer, pbuf, len,timestamp);
}

void rtp_muxer_destory(struct rtp_muxer_context_t* pContext)
{
	if (pContext != NULL)
	{
		if (pContext->video_muxer != NULL)
		{
			rtp_payload_encode_destroy(pContext->video_muxer);
			pContext->video_muxer = NULL;
		}
		if (pContext->audio_muxer != NULL)
		{
			rtp_payload_encode_destroy(pContext->audio_muxer);
			pContext->audio_muxer = NULL;
		}
		free(pContext);
		pContext = NULL;
	}
}
