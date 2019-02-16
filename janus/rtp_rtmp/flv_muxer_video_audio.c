#include "flv_muxer_video_audio.h"



void be_write_uint32(uint8_t* ptr, uint32_t val)
{
	ptr[0] = (uint8_t)((val >> 24) & 0xFF);
	ptr[1] = (uint8_t)((val >> 16) & 0xFF);
	ptr[2] = (uint8_t)((val >> 8) & 0xFF);
	ptr[3] = (uint8_t)(val & 0xFF);
}

int flv_write_tag(uint8_t* tag, uint8_t type, uint32_t bytes, uint32_t timestamp)
{
	// TagType
	tag[0] = type & 0x1F;

	// DataSize
	tag[1] = (bytes >> 16) & 0xFF;
	tag[2] = (bytes >> 8) & 0xFF;
	tag[3] = bytes & 0xFF;

	// Timestamp
	tag[4] = (timestamp >> 16) & 0xFF;
	tag[5] = (timestamp >> 8) & 0xFF;
	tag[6] = (timestamp >> 0) & 0xFF;
	tag[7] = (timestamp >> 24) & 0xFF; // Timestamp Extended

									   // StreamID(Always 0)
	tag[8] = 0;
	tag[9] = 0;
	tag[10] = 0;

	return 11;
}

static int flv_write_header(FILE* fp)
{
	uint8_t header[9 + 4];
	header[0] = 'F'; // FLV signature
	header[1] = 'L';
	header[2] = 'V';
	header[3] = 0x01; // File version
	header[4] = 0x05; // Type flags (audio & video)
	be_write_uint32(header + 5, 9); // Data offset
	be_write_uint32(header + 9, 0); // PreviousTagSize0(Always 0)
	if (sizeof(header) != fwrite(header, 1, sizeof(header), fp))
		return ferror(fp);
	return 0;
}

struct flv_muxer_context_t * flv_muxer_video_audio_init(void* parma, flv_muxer_cb cbfun)
{
	struct flv_muxer_context_t * pcontext = NULL;
	int flag = 0;
	do
	{
		pcontext = (struct flv_muxer_context_t*)calloc(1, sizeof(struct flv_muxer_context_t));
		if (pcontext == NULL)
		{
			JANUS_LOG(LOG_ERR, "Flv muxer context calloc flv_muxer_context_t failed, err = %d\n", errno);
			break;
		}
		pcontext->muxer = NULL;
		pcontext->cbdata = parma;
		pcontext->cbfun = cbfun;

#ifdef TEST_DEBUG
		pcontext->fd = fopen("/tmp/test.flv", "wb");
#endif // TEST_DEBUG

		pcontext->muxer = flv_muxer_create(cbfun, pcontext->cbdata);
		if (pcontext->muxer == NULL)
		{
			JANUS_LOG(LOG_ERR, "Flv muxer context cflv_muxer_create failed, err = %d\n", errno);
			break;
		}

#ifdef TEST_DEBUG
		flv_write_header(pcontext->fd);
#endif // TEST_DEBUG

		flag = 1;
		JANUS_LOG(LOG_INFO, "Flv muxer context init success.\n");
	} while (0);

	if (!flag)
	{
		flv_muxer_video_audio_destory(pcontext);
	}
	return pcontext;
}

int flv_muxer_video_audio_input(struct flv_muxer_context_t*pcontext, int avtype, int64_t pts, int64_t dts, const void* data, size_t bytes)
{
	if (PSI_STREAM_AAC == avtype || PSI_STREAM_MP3 == avtype) {
		if (0 == pcontext->a_s_pts)
			pcontext->a_s_pts = pts;
		pts -= pcontext->a_s_pts;
		dts -= pcontext->a_s_pts;
	}
	else if (PSI_STREAM_H264 == avtype || PSI_STREAM_H265 == avtype)
	{
		if (0 == pcontext->v_s_pts)
			pcontext->v_s_pts = pts;
		pts -= pcontext->v_s_pts;
		dts -= pcontext->v_s_pts;
	}
	else {
		assert(0);
	}

	if (PSI_STREAM_AAC == avtype)
	{
		flv_muxer_aac(pcontext->muxer, data, bytes, (uint32_t)(pts / 48), (uint32_t)(pts / 48));
	}
	else if (PSI_STREAM_MP3 == avtype)
	{
		flv_muxer_mp3(pcontext->muxer, data, bytes, (uint32_t)(pts / 48), (uint32_t)(pts / 48));
	}
	else if (PSI_STREAM_H264 == avtype)
	{
		flv_muxer_avc(pcontext->muxer, data, bytes, (uint32_t)(pts / 90), (uint32_t)(pts / 90));
	}
	else if (PSI_STREAM_H265 == avtype)
	{
		flv_muxer_hevc(pcontext->muxer, data, bytes, (uint32_t)(pts / 90), (uint32_t)(pts / 90));
	}
}

void flv_muxer_video_audio_destory(struct flv_muxer_context_t *pcontext)
{
	if (pcontext != NULL)
	{
		if (pcontext->muxer != NULL)
		{
			flv_muxer_destroy(pcontext->muxer);
			pcontext->muxer = NULL;
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
	JANUS_LOG(LOG_INFO, "Flv muxer context destroy\n");
}

