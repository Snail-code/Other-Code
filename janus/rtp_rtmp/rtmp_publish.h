#ifndef __RTMP_PULISH_H__
#define __RTMP_PULISH_H__

#include "sockutil.h"
#include "sys/system.h"
#include "rtmp-client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "debug.h"

struct rtmp_client_publish_context_t
{
	char  host[2 * 1024 * 1024];
	char  app[2 * 1024 * 1024];
	char  stream[2 * 1024 * 1024];
	char  packet[2 * 1024 * 1024];
	struct rtmp_client_t* rtmp;
	socket_t socket;
	int   aacconfig;
	int   avcrecord;
};

struct rtmp_client_publish_context_t* rtmp_client_init(const char* host, const char* app, const char* stream, int port, int timeout);
int rtmp_client_input_flv(struct rtmp_client_publish_context_t* pcontext,unsigned char* pdata,int len, int type, uint32_t timestamp);
void rtmp_client_context_destroy(struct rtmp_client_publish_context_t* pcontext);

#endif /*__RTMP_PULISH_H__*/