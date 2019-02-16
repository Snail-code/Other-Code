#include "flv_demuxer.h"


struct flv_demuxer_video_audio_context_t * flv_demuxer_video_audio_init(void* parma, flv_demuxer_video_audio_cb cbfun)
{
	struct flv_demuxer_video_audio_context_t * pcontext = NULL;
	int flag = 0;
	do 
	{
		pcontext = (struct flv_demuxer_video_audio_context_t*)calloc(1, sizeof(struct flv_demuxer_video_audio_context_t));
		if (!pcontext)
		{
			JANUS_LOG(LOG_ERR, "Flv demuxer init falied,when calloc context. err is %d\n", errno);
			break;
		}
		pcontext->cb = cbfun;
		pcontext->param = parma;
		pcontext->flv = flv_demuxer_create(pcontext->cb, pcontext);

		if (!pcontext->flv)
		{
			JANUS_LOG(LOG_ERR, "Flv demuxer init falied,when create demuxer. err is %d\n", errno);
			break;
		}
		pcontext->video_fd = fopen("/tmp/video.h264", "wb");
		pcontext->audio_fd = fopen("/tmp/audio.aac", "wb");
		JANUS_LOG(LOG_INFO, "Flv demuxer init success\n");
		flag = 1;
	} while (0);
	if (!flag)
	{
		flv_demuxer_video_audio_destory(pcontext);
	}
	return pcontext;
}

int flv_demuxer_video_audio_input(struct flv_demuxer_video_audio_context_t* demuxer, int type, const void* data, size_t bytes, uint32_t timestamp)
{
	return flv_demuxer_input(demuxer->flv, type, data, bytes, timestamp);
}

void flv_demuxer_video_audio_destory(struct flv_demuxer_video_audio_context_t *pcontext)
{
	if (pcontext != NULL)
	{
		if (pcontext->flv != NULL)
		{
			flv_demuxer_destroy(pcontext->flv);
			pcontext->flv = NULL;
		}
		free(pcontext);
		pcontext = NULL;
		JANUS_LOG(LOG_INFO, "Flv demuxer destory\n");
	}
}
