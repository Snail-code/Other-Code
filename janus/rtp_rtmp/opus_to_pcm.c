#include "opus_to_pcm.h"

struct opus_to_pcm_context_t * opus_to_pcm_init(int sampleRate, int channels, opus_to_pcm_cb cb, void* param)
{
	struct opus_to_pcm_context_t* pcontext = NULL;
	int flag = 0;
	do
	{
		pcontext = (struct opus_to_pcm_context_t*)calloc(1,sizeof(struct opus_to_pcm_context_t));
		if (pcontext ==NULL)
		{
			JANUS_LOG(LOG_ERR, "Opus decoder calloc opus_to_pcm_context_t failed ,err = %d\n", errno);
			break; 
		}
		int err;
		pcontext->decoder = opus_decoder_create(sampleRate, channels, &err);
		if (err != OPUS_OK)
		{
			JANUS_LOG(LOG_ERR, "Opus decoder create failed ,err = %d\n", errno);
			break;
		}
		pcontext->max_frame_size = 48000 * 2;
		pcontext->channels = channels;
		pcontext->out = (short*)malloc(pcontext->max_frame_size*channels * sizeof(short));
		pcontext->cb = cb;
		pcontext->cbdata = param;
		pcontext->src_seq = 0;

#ifdef TEST_DEBUG
		pcontext->fd = fopen("/tmp/pcm.pcm", "wb");
		if (pcontext->fd == NULL)
		{
			JANUS_LOG(LOG_ERR, "Opus decoder create failed ,err = %d\n", errno);
			break;
		}
#endif // TEST_DEBUG

		flag = 1;
		JANUS_LOG(LOG_INFO, "Opus decoder create success.\n");
	} while (0);

	if (!flag)
	{
		opus_to_pcm_destory(pcontext);
	}
	return pcontext;
}


int opus_to_pcm_decode(struct opus_to_pcm_context_t* pcontext,unsigned char* pdata, int len, uint32_t timestamp,int seq)
{
	if (pcontext ==NULL || pdata ==NULL)
	{
		JANUS_LOG(LOG_ERR, "Opus decoder input opus_to_pcm_context_t  or pdata is null\n");
		return -1;
	}

	/* We need to allocate for 16-bit PCM data, but we store it as unsigned char. */

	//TODO暂时fes功能
	int output_samples = pcontext->max_frame_size;
	output_samples = opus_decode(pcontext->decoder, pdata, len, pcontext->out, output_samples, 0);

	if (output_samples > 0)
	{
			int i;
			unsigned char* fbytes = (unsigned char*)malloc(output_samples*pcontext->channels * sizeof(short));
			for (i = 0; i < output_samples *pcontext->channels; i++)
			{
				short s;
				s = pcontext->out[i];
				fbytes[2 * i] = s & 0xFF;
				fbytes[2 * i + 1] = (s >> 8) & 0xFF;
			}
			pcontext->cb(pcontext->cbdata, fbytes, output_samples*pcontext->channels * sizeof(short), timestamp);
			free(fbytes);
	}
	return 1;
}


void opus_to_pcm_destory(struct opus_to_pcm_context_t* pcontext)
{
	if (pcontext != NULL)
	{
		opus_decoder_destroy(pcontext->decoder);
		if (pcontext->out != NULL)
		{
			free(pcontext->out);
			pcontext->out = NULL;
		}
#ifdef  TEST_DEBUG
		if (pcontext->fd != NULL)
		{
			fclose(pcontext->fd);
			pcontext->fd = NULL;
		}
#endif //  TEST_DEBUG

		free(pcontext);
		pcontext = NULL;
	}
	JANUS_LOG(LOG_INFO, "Opus decoder context destroy\n");
}
