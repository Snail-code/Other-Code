#include "rtmp_client_live_play.h"

void be_write_uint32(uint8_t* ptr, uint32_t val)
{
	ptr[0] = (uint8_t)((val >> 24) & 0xFF);
	ptr[1] = (uint8_t)((val >> 16) & 0xFF);
	ptr[2] = (uint8_t)((val >> 8) & 0xFF);
	ptr[3] = (uint8_t)(val & 0xFF);
}

static int rtmp_client_onaudio(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	struct rtmp_client_live_play_context_t* pcontext = (struct rtmp_client_live_play_context_t*) param;
	return pcontext->cbfun(param, data, bytes, timestamp, FLV_TYPE_AUDIO);
}

static int rtmp_client_onvideo(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	struct rtmp_client_live_play_context_t* pcontext = (struct rtmp_client_live_play_context_t*) param;
	return pcontext->cbfun(param, data, bytes, timestamp, FLV_TYPE_VIDEO);
}

static int rtmp_client_onscript(void* param, const void* data, size_t bytes, uint32_t timestamp)
{
	struct rtmp_client_live_play_context_t* pcontext = (struct rtmp_client_live_play_context_t*) param;
	return pcontext->cbfun(param, data, bytes, timestamp, FLV_TYPE_SCRIPT);
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

static int rtmp_client_send(void* param, const void* header, size_t len, const void* data, size_t bytes)
{
	struct rtmp_client_live_play_context_t*pcontext = (struct rtmp_client_live_play_context_t*)param;
	socket_bufvec_t vec[2];
	socket_setbufvec(vec, 0, (void*)header, len);
	socket_setbufvec(vec, 1, (void*)data, bytes);
	return socket_send_v_all_by_time(pcontext->socket, vec, bytes ? 2 : 1, 0, 2000);
}

struct rtmp_client_live_play_context_t* rtmp_client_live_play_init(const char* host, const char* app, const char* stream, int port, int timeout, void* param, rtmp_client_live_play_cb cbfun)
{

	struct rtmp_client_live_play_context_t* pcontext = NULL;
	int flag = 0;
	do
	{
		pcontext = (struct rtmp_client_live_play_context_t*)calloc(1, sizeof(struct rtmp_client_live_play_context_t));
		if (pcontext == NULL)
		{
			JANUS_LOG(LOG_ERR, "Rtmp client live play init failed, when calloc rtmp_client_live_play_context_t. err is %d\n", errno);
			break;
		}
		snprintf(pcontext->packet, sizeof(pcontext->packet), "rtmp://%s/%s", host, app); // tcurl
		snprintf(pcontext->host, sizeof(pcontext->host), "%s", host); // tcurl
		snprintf(pcontext->app, sizeof(pcontext->app), "%s", app); // tcurl
		snprintf(pcontext->stream, sizeof(pcontext->stream), "%s",stream); // tcurl
		pcontext->port = port;
		pcontext->timeout = timeout;
		pcontext->param = param;
		pcontext->cbfun = cbfun;
		
		JANUS_LOG(LOG_INFO, "Rtmp client live play init success.\n");
		flag = 1;
	} while (0);
	if (!flag)
	{
		rtmp_client_live_play_destroy(pcontext);
	}

	return pcontext;
}

int rtmp_client_live_play_connect(struct rtmp_client_live_play_context_t* pcontext)
{
	int ret = -1;
	socket_init();
	JANUS_LOG(LOG_INFO, "Rtmp client play rtmp://%s:%d/%s%s\n", pcontext->host, pcontext->port, pcontext->app, pcontext->stream);
	pcontext->socket = socket_connect_host(pcontext->host, pcontext->port, pcontext->timeout);
	if (pcontext->socket <= 0)
	{
		JANUS_LOG(LOG_ERR, "Rtmp client live play start failed, when connect server rtmp://%s:%d. err is %d\n", pcontext->host, pcontext->port, errno);
		return ret;
	}

	socket_setnonblock(pcontext->socket, 0);
	struct rtmp_client_handler_t handler;
	memset(&handler, 0, sizeof(handler));
	handler.send = rtmp_client_send;
	handler.onaudio = rtmp_client_onaudio;
	handler.onvideo = rtmp_client_onvideo;
	handler.onscript = rtmp_client_onscript;

	pcontext->rtmp = rtmp_client_create(pcontext->app, pcontext->stream, pcontext->packet/*tcurl*/, pcontext, &handler);
	if (pcontext->rtmp == NULL)
	{
		JANUS_LOG(LOG_ERR, "Rtmp client live play init failed, when rtmp client create. err is %d\n", errno);
		return  ret;
	}
	return rtmp_client_start(pcontext->rtmp, 1);
}

int  rtmp_client_live_play_start(struct rtmp_client_live_play_context_t* pcontext)
{

	int ret = socket_recv(pcontext->socket, pcontext->packet, sizeof(pcontext->packet), 0);
	rtmp_client_input(pcontext->rtmp, pcontext->packet, ret);
	return ret;
}

void rtmp_client_live_play_destroy(struct rtmp_client_live_play_context_t* pcontext)
{
	if (pcontext!=NULL)
	{
		rtmp_client_stop(pcontext->rtmp);
		if (pcontext->socket >0)
		{
			socket_close(pcontext->socket);
			pcontext->socket = -1;
		}
		socket_cleanup();
		rtmp_client_destroy(pcontext);
		pcontext = NULL;
	}
}
