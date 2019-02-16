#include "aac_decoder.h"

int aacdec_fill(struct aac_decoder_context_t* pcontext,char *data, int nb_data, int *pnb_left)
{
	pcontext->filled_bytes += nb_data;

	unsigned char *udata = (unsigned char *)data;
	unsigned int unb_data = (unsigned int)nb_data;
	unsigned int unb_left = unb_data;
	AAC_DECODER_ERROR err = aacDecoder_Fill(pcontext->dec, &udata, &unb_data, &unb_left);
	if (err != AAC_DEC_OK) {
		return err;
	}

	if (pnb_left) {
		*pnb_left = (int)unb_left;
	}

	return 0;
}

int aacdec_pcm_size(struct aac_decoder_context_t* pcontext)
{
	if (!pcontext->info) {
		return 0;
	}
	return (int)(pcontext->info->numChannels * pcontext->info->frameSize * pcontext->sample_bits / 8);
}

int aacdec_decode_frame(struct aac_decoder_context_t* pcontext, char *pcm, int nb_pcm, int *pnb_valid)
{
	if (pcontext->is_adts && pcontext->info && pcontext->filled_bytes - pcontext->info->numTotalBytes <= 7) {
		return AAC_DEC_NOT_ENOUGH_BITS;
	}

	INT_PCM* upcm = (INT_PCM*)pcm;
	int unb_pcm = (int)nb_pcm;
	AAC_DECODER_ERROR err = aacDecoder_DecodeFrame(pcontext->dec, upcm, unb_pcm, 0);
	// user should fill more bytes then decode.
	if (err == AAC_DEC_NOT_ENOUGH_BITS) {
		return err;
	}
	if (err != AAC_DEC_OK) {
		return err;
	}

	// when decode ok, retrieve the info.
	if (!pcontext->info) {
		pcontext->info = aacDecoder_GetStreamInfo(pcontext->dec);
	}

	// the actual size of pcm.
	if (pnb_valid) {
		*pnb_valid = aacdec_pcm_size(pcontext);
	}

	return 0;
}


struct aac_decoder_context_t* aac_decoder_init(int channle, int sample, void* param, aac_decoder_cb cb)
{
	struct aac_decoder_context_t* pcontext = NULL;
	int flag = 0;
	do 
	{
		pcontext = (struct aac_decoder_context_t*)calloc(1, sizeof(struct aac_decoder_context_t));
		if (pcontext  == NULL)
		{
			JANUS_LOG(LOG_ERR, "aac decoder init context failed, when calloc aac_decoder_context_t struct.\n");
			break;
		}
		pcontext->param = param;
		pcontext->cbfun = cb;
		pcontext->channles = channle;
		pcontext->sample = sample;

		pcontext->sample_bits = 16;
		pcontext->is_adts = 1;
		pcontext->filled_bytes = 0;

		pcontext->dec = aacDecoder_Open(TT_MP4_ADTS, 1);
		if (pcontext->dec == NULL)
		{
			JANUS_LOG(LOG_ERR, "aac decoder init context failed, when open aac decoder.\n");
			break;
		}
		pcontext->pcm_buf = (char*)calloc(1, DECODE_PCM_BUF_LEN);
		pcontext->pcm_buf_len = DECODE_PCM_BUF_LEN;
		pcontext->info = NULL;
		flag = 1;

		pcontext->pcm_fd = fopen("/tmp/pcm.pcm", "wb");
		JANUS_LOG(LOG_INFO, "aac decoder init context success.\n");
	} while (0);
	if (!flag)
	{
		aac_decoder_destroy(pcontext);
	}
	return pcontext;
}

int aac_decoder_input(struct aac_decoder_context_t* pcontext, const char* pdata, int bytes, int timestamp)
{
	struct adts_header_t *adts = (struct adts_header_t *)(pdata);
	if (adts->syncword_0_to_8 != 0xff || adts->syncword_9_to_12 != 0xf) {
		return -1;
	}

	int aac_frame_size = adts->frame_length_0_to_1 << 11 | adts->frame_length_2_to_9 << 3 | adts->frame_length_10_to_12;
	if (aac_frame_size > bytes) {
		return -1;
	}

	int leftSize = aac_frame_size;
	int ret = aacdec_fill(pcontext, pdata, aac_frame_size, &leftSize);
	if (ret != 0) {
		return -1;
	}

	if (leftSize > 0) {
		return -1;
	}

	int validSize = 0;
	ret = aacdec_decode_frame(pcontext,pcontext->pcm_buf, DECODE_PCM_BUF_LEN, &validSize);
	if (ret == AAC_DEC_NOT_ENOUGH_BITS) {
		return -1;
	}

	if (ret != 0) {
		return -1;
	}
	pcontext->cbfun(pcontext, pcontext->pcm_buf, validSize, timestamp);
}

void aac_decoder_destroy(struct aac_decoder_context_t* pcontext)
{
	if (!pcontext)
	{
		if (pcontext->dec != NULL)
		{
			aacDecoder_Close(pcontext->dec);
			pcontext->dec = NULL;
		}
		if (pcontext->pcm_buf !=NULL)
		{
			free(pcontext->pcm_buf);
		}
		free(pcontext);
		pcontext = NULL;
		JANUS_LOG(LOG_INFO, "aac decoder destroy context success.\n");
	}
}


