#include "opus_encoder.h"
#include "../debug.h"

struct opus_encode_context_t* pcm_to_opus_encode_init(int sampleRate, int channels, int family, opus_encode_cb cb, void* param)
{
	struct opus_encode_context_t* pcontext = NULL;
	int flag = 0;
	do
	{
		pcontext = (struct opus_encode_context_t*)calloc(1, sizeof(struct opus_encode_context_t));
		if (pcontext == NULL)
		{
			assert(0);
			break;
		}
		int error;

		pcontext->Enc = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);
		if (error != OPUS_OK || pcontext->Enc == NULL)
		{
			assert(0);
			break;
		}
		pcontext->input_buf_capacity = INPUT_BUFFER_CHAR_SIZE;
		pcontext->inputbuffer = (char*)malloc(pcontext->input_buf_capacity);
		pcontext->convert_buf = (uint16_t*)malloc(pcontext->input_buf_capacity);

		pcontext->used_len = 0;

		opus_encoder_ctl(pcontext->Enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_BITRATE(OPUS_AUTO)); 
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_VBR(1));//0:CBR, 1:VBR
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_VBR_CONSTRAINT(0));//0:Unconstrained VBR., 1:Constrained VBR.
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_COMPLEXITY(5));//range:0~10
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_FORCE_CHANNELS(2)); //1:Forced mono, 2:Forced stereo
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));//
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_INBAND_FEC(1));//0:Disable, 1:Enable
		opus_encoder_ctl(pcontext->Enc, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS));


		pcontext->cb = cb;
		pcontext->param = param;
		pcontext->timestamp = 0;
		flag = 1;
	} while (0);
	return pcontext;
}

int  pcm_to_opus_encode_input(struct opus_encode_context_t* pcontext, unsigned char* pdata, int len, uint32_t timestamp)
{
	unsigned char* ptr = pdata;
	pcontext->timestamp = timestamp;
	char buf[1500];
	int n = 0;
	do {
		int need = (len - (ptr - pdata)) < ((INPUT_BUFFER_CHAR_SIZE) - pcontext->used_len%(INPUT_BUFFER_CHAR_SIZE))?
			(len - (ptr - pdata)) : ((INPUT_BUFFER_CHAR_SIZE)-pcontext->used_len % (INPUT_BUFFER_CHAR_SIZE));
		memcpy(pcontext->inputbuffer + pcontext->used_len, ptr, need);
		pcontext->used_len += need;
		ptr += need;
		
		assert(pcontext->used_len <= INPUT_BUFFER_CHAR_SIZE);
		if (pcontext->used_len >= INPUT_BUFFER_CHAR_SIZE) {
			int ret = opus_encode(pcontext->Enc, pcontext->inputbuffer, 960 * 4, buf, 1500);
			if (ret > 0) {
				pcontext->cb(pcontext, buf, ret, pcontext->timestamp );
			}
			else {
				JANUS_LOG(LOG_VERB, "opus_encode error %s pcm data len %d\n", opus_strerror(ret), pcontext->used_len);
			}
		}
		else {
			break;
		}

		pcontext->used_len = 0;

	} while (pcontext->used_len + (pdata +len - ptr) >= INPUT_BUFFER_CHAR_SIZE);

	if ((len - (ptr - pdata)) > 0)
	{
		memcpy(pcontext->inputbuffer, ptr, (len - (ptr - pdata)));
		pcontext->used_len += (len - (ptr - pdata));
		assert(pcontext->used_len < INPUT_BUFFER_CHAR_SIZE);
	}

	return 0;
}

void pcm_to_opus_encode_destory(struct opus_encode_context_t* pcontext)
{

}




