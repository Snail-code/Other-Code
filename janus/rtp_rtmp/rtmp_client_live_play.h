#ifndef __RTMP_CLIENT_LIVE_PLAY__H__
#define __RTMP_CLIENT_LIVE_PLAY__H__
#include "sockutil.h"
#include "rtmp-client.h"
#include <assert.h>
#include "flv-proto.h"
#include <stdio.h>
#include <stdint.h>
#include "debug.h"
#include <errno.h>

typedef int(*rtmp_client_live_play_cb)(void* param, const unsigned char* data, size_t bytes, uint32_t timestamp,int type);


struct rtmp_client_live_play_context_t
{
	char  host[2 * 1024 * 1024];
	char  app[2 * 1024 * 1024];
	char  stream[2 * 1024 * 1024];
	char  packet[2 * 1024 * 1024];
	struct rtmp_client_t* rtmp;
	void*  param;
	rtmp_client_live_play_cb cbfun;
	socket_t socket;
	int port;
	int timeout;
};


struct rtmp_client_live_play_context_t* rtmp_client_live_play_init(const char* host, const char* app, const char* stream, int port, int timeout,void* param, rtmp_client_live_play_cb cbfun);
int rtmp_client_live_play_connect(struct rtmp_client_live_play_context_t* pcontext);
int rtmp_client_live_play_start(struct rtmp_client_live_play_context_t* pcontext);
void rtmp_client_live_play_destroy(struct rtmp_client_live_play_context_t* pcontext);


#endif // !__RTMP_CLIENT_LIVE_PLAY__H__


