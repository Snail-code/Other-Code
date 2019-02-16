#include "rtmp_publish.h"
#include "libflv/include/flv-proto.h"

//#define CORRUPT_RTMP_CHUNK_DATA
#if defined(CORRUPT_RTMP_CHUNK_DATA)
static void rtmp_corrupt_data(const void* data, size_t bytes)
{
	static unsigned int seed;
	if (0 == seed)
	{
		seed = (unsigned int)time(NULL);
		srand(seed);
	}

	if (bytes < 1)
		return;

	//size_t i = bytes > 20 ? 20 : bytes;
	//i = rand() % i;

	//uint8_t v = ((uint8_t*)data)[i];
	//((uint8_t*)data)[i] = rand() % 255;
	//printf("rtmp_corrupt_data[%d] %d == %d\n", i, (int)v, (int)((uint8_t*)data)[i]);

	if (5 == rand() % 10)
	{
		size_t i = rand() % bytes;
		uint8_t v = ((uint8_t*)data)[i];
		((uint8_t*)data)[i] = rand() % 255;
		printf("rtmp_corrupt_data[%d] %d == %d\n", i, (int)v, (int)((uint8_t*)data)[i]);
	}
}

static uint8_t s_buffer[4 * 1024 * 1024];
static size_t s_offset;
static FILE* s_fp;
static void fwritepacket(uint32_t timestamp)
{
	assert(4 == fwrite(&s_offset, 1, 4, s_fp));
	assert(4 == fwrite(&timestamp, 1, 4, s_fp));
	assert(s_offset == fwrite(s_buffer, 1, s_offset, s_fp));
	s_offset = 0;
}
#endif

static int rtmp_client_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	socket_t* socket = (socket_t*)param;
	socket_bufvec_t vec[2];
	socket_setbufvec(vec, 0, (void*)header, len);
	socket_setbufvec(vec, 1, (void*)data, bytes);

#if defined(CORRUPT_RTMP_CHUNK_DATA)
	//if (len > 0)
	//{
	//    assert(s_offset + len < sizeof(s_buffer));
	//    memcpy(s_buffer + s_offset, header, len);
	//    s_offset += len;
	//}
	//if (bytes > 0)
	//{
	//    assert(s_offset + bytes < sizeof(s_buffer));
	//    memcpy(s_buffer + s_offset, data, bytes);
	//    s_offset += bytes;
	//}

	rtmp_corrupt_data(header, len);
	rtmp_corrupt_data(data, bytes);
#endif
	return socket_send_v_all_by_time(*socket, vec, bytes > 0 ? 2 : 1, 0, 5000);
}

struct rtmp_client_publish_context_t*  rtmp_client_init(const char* host, const char* app, const char* stream,int port, int timeout)
{
	struct rtmp_client_publish_context_t* pContext = NULL;
	int flag = 0;
	do 
	{
		pContext = (struct rtmp_client_publish_context_t*)calloc(1, sizeof(struct rtmp_client_publish_context_t));
		if (pContext == NULL)
		{
			JANUS_LOG(LOG_ERR, "Rtmp client init failed, when calloc rtmp_client_context_t. err is %d\n",errno);
			break;
		}
		snprintf(pContext->packet, sizeof(pContext->packet), "rtmp://%s/%s", host, app); // tcurl
		snprintf(pContext->host, sizeof(pContext->host), "%s", host); // tcurl
		snprintf(pContext->app, sizeof(pContext->app), "%s", app); // tcurl
		snprintf(pContext->stream, sizeof(pContext->stream), "%s",stream); // tcur

		pContext->aacconfig = 0;
		pContext->avcrecord = 0;

		struct rtmp_client_handler_t handler;
		memset(&handler, 0, sizeof(handler));
		handler.send = rtmp_client_send;
		
		socket_init();
		pContext->socket = socket_connect_host(host, port, timeout);
		socket_setnonblock(pContext->socket, 0);
		pContext->rtmp= rtmp_client_create(app, stream, pContext->packet/*tcurl*/, &pContext->socket, &handler);
		if (pContext->rtmp ==NULL)
		{
			JANUS_LOG(LOG_ERR, "Rtmp client init failed, when create rtmp client. err is %d\n", errno);
			break;
		}
		//// 0-publish, 1-live/vod, 2-live only, 3-vod only
		int r = rtmp_client_start(pContext->rtmp, 0);
		if (r <0)
		{
			JANUS_LOG(LOG_ERR, "Rtmp client init failed, when start client. err is %d\n", errno);
			break;
		}
		while (4 != rtmp_client_getstate(pContext->rtmp) && (r = socket_recv(pContext->socket, pContext->packet, sizeof(pContext->packet), 0)) > 0)
		{
			r = rtmp_client_input(pContext->rtmp, pContext->packet, r);
		}
		flag = 1;
		JANUS_LOG(LOG_INFO, "Init rtmp client success.\n");
	} while (0);
	
	if (!flag)
	{
		rtmp_client_context_destroy(pContext);
	}
	return pContext;
}

int rtmp_client_input_flv(struct rtmp_client_publish_context_t* pcontext, unsigned char* packet, int len, int type, uint32_t timestamp)
{
	int nRet;
	if (FLV_TYPE_AUDIO == type)
	{
		if (0 == packet[1])
		{
			if (0 != pcontext->aacconfig)
			{
				return -1;
			}
			pcontext->aacconfig = 1;
		}
		nRet = rtmp_client_push_audio(pcontext->rtmp, packet, len, timestamp);
	}
	else if (FLV_TYPE_VIDEO == type)
	{
		if (0 == packet[1] || 2 == packet[1])
		{
			if (0 != pcontext->avcrecord)
			{
				return -1;
			}
			pcontext->avcrecord = 1;
		}
		nRet = rtmp_client_push_video(pcontext->rtmp, packet, len, timestamp);
	}
	else if (FLV_TYPE_SCRIPT == type)
	{
		nRet = rtmp_client_push_script(pcontext->rtmp, packet, len, timestamp);
	}
	return 0;
}

void rtmp_client_context_destroy(struct rtmp_client_publish_context_t* pcontext)
{
	if (pcontext!=NULL)
	{
		if (pcontext->rtmp!=NULL)
		{
			rtmp_client_destroy(pcontext->rtmp);
		}
		socket_close(pcontext->socket);
		socket_cleanup();
	}
	JANUS_LOG(LOG_INFO, "RTMP client context destroy\n");
}
