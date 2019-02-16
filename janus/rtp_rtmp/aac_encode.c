#include "aac_encode.h"


/*
Supported AOTs:
2	AAC-LC
5	HE-AAC
29	HE-AAC v2
23	AAC-LD
39	AAC-ELD
*/
struct  aac_encode_context_t* aac_encoder_init(int channle, int sample_rate, int format, int aot, int vbr, pcm_encode_cb cbfun, void* cbdata)
{
	struct aac_encode_context_t* pcontext = NULL;
	int flag = 0;
	do
	{
		pcontext = (struct aac_encode_context_t*)calloc(1, sizeof(struct aac_encode_context_t));
		if (pcontext == NULL)
		{
			JANUS_LOG(LOG_ERR, "AAC encoder calloc aac_encode_context_t failed, err = %d\n", errno);
			break;
		}
		pcontext->afterburner = 1;
		pcontext->aot = aot;
		pcontext->bitrate = 64000;
		pcontext->bits_per_sample = channle*sample_rate*format;
		pcontext->ch = 0;
		pcontext->channels = channle;
		pcontext->eld_sbr = 0;
		pcontext->format = format;
		pcontext->handle = NULL;
		pcontext->info;
		pcontext->mode = 0;
		pcontext->sample_rate = sample_rate;
		pcontext->vbr = vbr;
		pcontext->cbfun = cbfun;
		pcontext->cbdata = cbdata;
		pcontext->usedlen = 0;
		pcontext->convert_buf = NULL;

		switch (pcontext->channels) {
		case 1: pcontext->mode = MODE_1;       break;
		case 2: pcontext->mode = MODE_2;       break;
		case 3: pcontext->mode = MODE_1_2;     break;
		case 4: pcontext->mode = MODE_1_2_1;   break;
		case 5: pcontext->mode = MODE_1_2_2;   break;
		case 6: pcontext->mode = MODE_1_2_2_1; break;
		default:
			JANUS_LOG(LOG_ERR, "Unsupported WAV channels %d\n", pcontext->channels);
			break;
		}
		if (aacEncOpen(&pcontext->handle, 0, pcontext->channels) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to open encoder\n");
			break;
		}

		/*if (aot == 39 && eld_sbr) {
			if (aacEncoder_SetParam(pcontext->handle, AACENC_SBR_MODE, 1) != AACENC_OK) {
				fprintf(stderr, "Unable to set SBR mode for ELD\n");
				break;
			}
		}*/
		if (aacEncoder_SetParam(pcontext->handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to set the AOT\n");
			break;
		}

		if (aacEncoder_SetParam(pcontext->handle, AACENC_CHANNELMODE, pcontext->mode) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to set the channel mode\n");
			break;
		}

		if (aacEncoder_SetParam(pcontext->handle, AACENC_CHANNELORDER, 1) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to set the wav channel order\n");
			break;
		}

		if (vbr) {
			if (aacEncoder_SetParam(pcontext->handle, AACENC_BITRATEMODE, vbr) != AACENC_OK) {
				JANUS_LOG(LOG_ERR, "Unable to set the VBR bitrate mode\n");
				break;
			}
		}
		else {
			if (aacEncoder_SetParam(pcontext->handle, AACENC_BITRATE, pcontext->bitrate) != AACENC_OK) {
				JANUS_LOG(LOG_ERR, "Unable to set the bitrate\n");
				break;
			}
		}

		if (aacEncoder_SetParam(pcontext->handle, AACENC_TRANSMUX, 2) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to set the ADTS transmux\n");
			break;
		}

		if (aacEncoder_SetParam(pcontext->handle, AACENC_AFTERBURNER, pcontext->afterburner) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to set the afterburner mode\n");
			break;
		}
	
		if (aacEncEncode(pcontext->handle, NULL, NULL, NULL, NULL) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to initialize the encoder\n");
			break;
		}

		if (aacEncInfo(pcontext->handle, &pcontext->info) != AACENC_OK) {
			JANUS_LOG(LOG_ERR, "Unable to get the encoder info\n");
			break;
		}
		pcontext->input_size = pcontext->channels * 2 * pcontext->info.frameLength;
		if (pcontext->input_size <= 0)
		{
			JANUS_LOG(LOG_ERR, "Get input size is %d\n", pcontext->input_size);
			assert(0);
			break;
		}

		pcontext->input_buf = (uint8_t*)malloc(pcontext->input_size);
		pcontext->convert_buf = (int16_t*)malloc(pcontext->input_size);
		pcontext->usedlen = 0;
		pcontext->convert_buf[0] = 1;
		if (pcontext->input_buf == NULL || pcontext->convert_buf == NULL)
		{
			JANUS_LOG(LOG_ERR, "ACC encoder malloc input_buf or convert_buf failed, err = %d\n",errno);
			break;
		}
#ifdef TEST_DEBUG
		pcontext->fd = fopen("/tmp/out.aac", "wb");
#endif // TEST_DEBUG

		JANUS_LOG(LOG_INFO, "Init aac encoder success \n");
		flag = 1;
	} while (0);

	if (!flag)
	{
		aac_encode_destory(pcontext);
	}
	return pcontext;
}

int aac_encode_input(struct aac_encode_context_t* pcontext, const unsigned char* pdata, int len, uint32_t timestamp)
{
	int emptylen = pcontext->input_size - pcontext->usedlen;
	if (emptylen > len)
	{
		memcpy(pcontext->input_buf + pcontext->usedlen, pdata, len);
		pcontext->usedlen += len;
		pcontext->timestamp = timestamp;
		return 1;
	}
	else
	{
		memcpy(pcontext->input_buf + pcontext->usedlen, pdata, emptylen);
		pcontext->usedlen += emptylen;
	}

	AACENC_BufDesc in_buf = { 0 }, out_buf = { 0 };
	AACENC_InArgs in_args = { 0 };
	AACENC_OutArgs out_args = { 0 };
	int in_identifier = IN_AUDIO_DATA;
	int in_size, in_elem_size;
	int out_identifier = OUT_BITSTREAM_DATA;
	int out_size, out_elem_size;
	int  i;
	void *in_ptr, *out_ptr;
	uint8_t outbuf[20480];
	AACENC_ERROR err;

	for (i = 0; i < pcontext->input_size / 2; i++)
	{
		pcontext->convert_buf[i] = pcontext->input_buf[2*i] | (pcontext->input_buf[2*i+1] << 8);
	}
	in_ptr = pcontext->convert_buf;
	in_size = pcontext->input_size;
	in_elem_size = 2;

	in_args.numInSamples = pcontext->input_size / 2;
	in_buf.numBufs = 1;
	in_buf.bufs = &in_ptr;
	in_buf.bufferIdentifiers = &in_identifier;
	in_buf.bufSizes = &in_size;
	in_buf.bufElSizes = &in_elem_size;

	out_ptr = outbuf;
	out_size = sizeof(outbuf);
	out_elem_size = 1;
	out_buf.numBufs = 1;
	out_buf.bufs = &out_ptr;
	out_buf.bufferIdentifiers = &out_identifier;
	out_buf.bufSizes = &out_size;
	out_buf.bufElSizes = &out_elem_size;

	if ((err = aacEncEncode(pcontext->handle, &in_buf, &out_buf, &in_args, &out_args)) != AACENC_OK) {
		if (err == AACENC_ENCODE_EOF)
		{
			JANUS_LOG(LOG_ERR, "AAC encoding failed\n");
			return 1;
		}
	}
	if (out_args.numOutBytes <= 0)
	{
		JANUS_LOG(LOG_ERR, "AAC encoding failed, out_args.numOutBytes is %d\n", out_args.numOutBytes);
		return 1;
	}
	pcontext->cbfun(pcontext->cbdata, (unsigned char*)outbuf, out_args.numOutBytes, (pcontext->timestamp+timestamp)/2);
	pcontext->usedlen = 0;
	pcontext->timestamp = timestamp;
	
	if (len - emptylen > 0)
	{
		memcpy(pcontext->input_buf, pdata + emptylen, len - emptylen);
		pcontext->timestamp = timestamp;
		pcontext->usedlen += len - emptylen;
	}
}

void aac_encode_destory(struct aac_encode_context_t* pcontext)
{
	if (pcontext!=NULL)
	{
		if (pcontext->input_buf!=NULL)
		{
			free(pcontext->input_buf);
			pcontext->input_buf = NULL;
		}
		if (pcontext->convert_buf!=NULL)
		{
			free(pcontext->convert_buf);
			pcontext->convert_buf = NULL;
		}
#ifdef TEST_DEBUG
		if (pcontext->fd != NULL)
		{
			fclose(pcontext->fd);
			pcontext->fd = NULL;
		}
#endif // TEST_DEBUG
		free(pcontext);
		pcontext = NULL;
	}
	JANUS_LOG(LOG_INFO, "AAC encoder context destroy\n");
}

