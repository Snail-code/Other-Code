#pragma execution_character_set("utf-8")
#include "rtp_to_opus.h"

/* helper, write a little-endian 32 bit int to memory */
void le32(unsigned char *p, int v)
{
	p[0] = v & 0xff;
	p[1] = (v >> 8) & 0xff;
	p[2] = (v >> 16) & 0xff;
	p[3] = (v >> 24) & 0xff;
}

/* helper, write a little-endian 16 bit int to memory */
void le16(unsigned char *p, int v)
{
	p[0] = v & 0xff;
	p[1] = (v >> 8) & 0xff;
}

/* helper, write a big-endian 32 bit int to memory */
void be32(unsigned char *p, int v)
{
	p[0] = (v >> 24) & 0xff;
	p[1] = (v >> 16) & 0xff;
	p[2] = (v >> 8) & 0xff;
	p[3] = v & 0xff;
}

/* helper, write a big-endian 16 bit int to memory */
void be16(unsigned char *p, int v)
{
	p[0] = (v >> 8) & 0xff;
	p[1] = v & 0xff;
}

/* manufacture a generic OpusHead packet */
ogg_packet *op_opushead(int samplerate, int channels)
{
	int size = 19;
	unsigned char *data = malloc(size);
	ogg_packet *op = malloc(sizeof(*op));

	if (!data) {
		fprintf(stderr, "Couldn't allocate data buffer.\n");
		free(op);
		return NULL;
	}
	if (!op) {
		fprintf(stderr, "Couldn't allocate Ogg packet.\n");
		free(data);
		return NULL;
	}

	memcpy(data, "OpusHead", 8);  /* identifier */
	data[8] = 1;                  /* version */
	data[9] = channels;           /* channels */
	le16(data + 10, 0);             /* pre-skip */
	le32(data + 12, samplerate);  /* original sample rate */
	le16(data + 16, 0);           /* gain */
	data[18] = 0;                 /* channel mapping family */

	op->packet = data;
	op->bytes = size;
	op->b_o_s = 1;
	op->e_o_s = 0;
	op->granulepos = 0;
	op->packetno = 0;

	return op;
}

/* manufacture a generic OpusTags packet */
ogg_packet *op_opustags(void)
{
	char *identifier = "OpusTags";
	char *vendor = "opus rtp packet dump";
	int size = strlen(identifier) + 4 + strlen(vendor) + 4;
	unsigned char *data = malloc(size);
	ogg_packet *op = malloc(sizeof(*op));

	if (!data) {
		fprintf(stderr, "Couldn't allocate data buffer.\n");
		free(op);
		return NULL;
	}
	if (!op) {
		fprintf(stderr, "Couldn't allocate Ogg packet.\n");
		free(data);
		return NULL;
	}

	memcpy(data, identifier, 8);
	le32(data + 8, strlen(vendor));
	memcpy(data + 12, vendor, strlen(vendor));
	le32(data + 12 + strlen(vendor), 0);

	op->packet = data;
	op->bytes = size;
	op->b_o_s = 0;
	op->e_o_s = 0;
	op->granulepos = 0;
	op->packetno = 1;

	return op;
}

ogg_packet *op_from_pkt(const unsigned char *pkt, int len)
{
	ogg_packet *op = malloc(sizeof(*op));
	if (!op) {
		fprintf(stderr, "Couldn't allocate Ogg packet.\n");
		return NULL;
	}

	op->packet = (unsigned char *)pkt;
	op->bytes = len;
	op->b_o_s = 0;
	op->e_o_s = 0;

	return op;
}

/* free a packet and its contents */
void op_free(ogg_packet *op)
{
	if (op) {
		if (op->packet) {
			free(op->packet);
		}
		free(op);
	}
}

/* check if an ogg page begins an opus stream */
int is_opus(ogg_page *og)
{
	ogg_stream_state os;
	ogg_packet op;

	ogg_stream_init(&os, ogg_page_serialno(og));
	ogg_stream_pagein(&os, og);
	if (ogg_stream_packetout(&os, &op) == 1) {
		if (op.bytes >= 19 && !memcmp(op.packet, "OpusHead", 8)) {
			ogg_stream_clear(&os);
			return 1;
		}
	}
	ogg_stream_clear(&os);
	return 0;
}

/* helper, write out available ogg pages */
int ogg_opus_write(struct rtp_opus_context_t *params)
{
	ogg_page page;
	size_t written;

	if (!params || !params->stream || !params->cbfun) {
		JANUS_LOG(LOG_ERR, "Rtp to opus rtp_opus_context_t is null or ogg_opus_stream is null or cbfun is null.\n");
		return -1;
	}

	while (ogg_stream_pageout(params->stream, &page)) {
		params->cbfun(params, page.header, page.header_len, params->timestamp, params->seq);
		params->cbfun(params, page.body, page.body_len, params->timestamp, params->seq);
	}
	return 0;
}

#define RTP_HEADER_MIN 12
typedef struct {
	int version;
	int type;
	int pad, ext, cc, mark;
	int seq, time;
	int ssrc;
	int *csrc;
	int header_size;
	int payload_size;
} rtp_header;

/* helper, read a big-endian 16 bit int from memory */
static int rbe16(const unsigned char *p)
{
	int v = p[0] << 8 | p[1];
	return v;
}

/* helper, read a big-endian 32 bit int from memory */
static int rbe32(const unsigned char *p)
{
	int v = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
	return v;
}

/* helper, read a native-endian 32 bit int from memory */
static int rne32(const unsigned char *p)
{
	/* On x86 we could just cast, but that might not meet
	* arm alignment requirements. */
	int d = 0;
	memcpy(&d, p, 4);
	return d;
}

int parse_rtp_header(const unsigned char *packet, int size, rtp_header *rtp)
{
	if (!packet || !rtp) {
		return -2;
	}
	if (size < RTP_HEADER_MIN) {
		fprintf(stderr, "Packet too short for rtp\n");
		return -1;
	}
	rtp->version = (packet[0] >> 6) & 3;
	rtp->pad = (packet[0] >> 5) & 1;
	rtp->ext = (packet[0] >> 4) & 1;
	rtp->cc = packet[0] & 7;
	rtp->header_size = 12 + 4 * rtp->cc;
	if (rtp->ext == 1) {
		uint16_t ext_length;
		rtp->header_size += 4;
		ext_length = rbe16(packet + rtp->header_size - 2);
		rtp->header_size += ext_length * 4;
	}
	rtp->payload_size = size - rtp->header_size;

	rtp->mark = (packet[1] >> 7) & 1;
	rtp->type = (packet[1]) & 127;
	rtp->seq = rbe16(packet + 2);
	rtp->time = rbe32(packet + 4);
	rtp->ssrc = rbe32(packet + 8);
	rtp->csrc = NULL;
	if (size < rtp->header_size) {
		fprintf(stderr, "Packet too short for RTP header\n");
		return -1;
	}

	return 0;
}

int serialize_rtp_header(unsigned char *packet, int size, rtp_header *rtp)
{
	int i;

	if (!packet || !rtp) {
		return -2;
	}
	if (size < RTP_HEADER_MIN) {
		fprintf(stderr, "Packet buffer too short for RTP\n");
		return -1;
	}
	if (size < rtp->header_size) {
		fprintf(stderr, "Packet buffer too short for declared RTP header size\n");
		return -3;
	}
	packet[0] = ((rtp->version & 3) << 6) |
		((rtp->pad & 1) << 5) |
		((rtp->ext & 1) << 4) |
		((rtp->cc & 7));
	packet[1] = ((rtp->mark & 1) << 7) |
		((rtp->type & 127));
	be16(packet + 2, rtp->seq);
	be32(packet + 4, rtp->time);
	be32(packet + 8, rtp->ssrc);
	if (rtp->cc && rtp->csrc) {
		for (i = 0; i < rtp->cc; i++) {
			be32(packet + 12 + i * 4, rtp->csrc[i]);
		}
	}

	return 0;
}

int update_rtp_header(rtp_header *rtp)
{
	rtp->header_size = 12 + 4 * rtp->cc;
	return 0;
}

int ogg_opus_flush(struct rtp_opus_context_t *params)
{
	ogg_page page;
	size_t written;

	if (!params || !params->stream||!params->cbfun) {
		JANUS_LOG(LOG_ERR, "Rtp to opus rtp_opus_context_t is null or ogg_opus_stream is null or cbfun is null.\n");
		return -1;
	}

	while (ogg_stream_flush(params->stream, &page)) {
		params->cbfun(params, page.header, page.header_len, params->timestamp, params->seq);
		params->cbfun(params, page.body, page.body_len, params->timestamp, params->seq);
	}
	return 0;
}

//rtp音频上下文初始化
struct rtp_opus_context_t*  rtp_opus_decode_init(
	int opus_payload_type, 
	const char* opus_payload_name,
	opus_decode_packet_cb packet,
	void* cbdata,
	int opus_sample,
	int opus_channel)
{
	struct rtp_opus_context_t *params = NULL;
	int status = 0;
	do
	{
		params = malloc(sizeof(struct rtp_opus_context_t));
		if (!params) {
			JANUS_LOG(LOG_ERR, "Init decode rtp audio context filed, when malloc rtp_opus_context_t err is %d.\n", errno);
			break;
		}
		params->stream = malloc(sizeof(ogg_stream_state));
		if (!params->stream) {
			JANUS_LOG(LOG_ERR, "Init decode rtp audio context filed, when malloc ogg_stream_state err is %d.\n", errno);
			break;
		}
		if (ogg_stream_init(params->stream, rand()) < 0) {
			JANUS_LOG(LOG_ERR, "Init decode rtp audio context filed, when init ogg_stream. err is %d.\n", errno);
			break;
		}

		params->seq = 0;
		params->granulepos = 0;
		params->payload_type = opus_payload_type;
		params->cbfun = packet;
		params->cbdata = cbdata;
		params->channle = opus_channel;
		params->sample = opus_sample;
#ifdef TEST_DEBUG
		params->fp = fopen("/tmp/opus_out.opus", "wb");
		if (!params->fp) {
			JANUS_LOG(LOG_ERR, "Init decode rtp audio context filed, when open opus file. err is %d.\n", errno);
			break;
		}
#endif // !

		ogg_packet *op;
		op = op_opushead(opus_sample, opus_channel);
		ogg_stream_packetin(params->stream, op);
		op_free(op);
		op = op_opustags();
		ogg_stream_packetin(params->stream, op);
		op_free(op);
		ogg_opus_flush(params);
		status = 1;
		JANUS_LOG(LOG_INFO, "Init decode rtp audio context success.\n");
	} while (0);

	if (!status)
	{
		if (!params)
		{
			if (!params->stream)
			{
				ogg_stream_destroy(params->stream);
				params->stream = NULL;
			}

#ifdef TEST_DEBUG
			if (!params->fp)
			{
				fclose(params->fp);
				params->fp = NULL;
			}
#endif // TEST_DEBUG

			free(params);
			params = NULL;
		}
	}
	return params;
}

int rtp_opus_decode_input(struct rtp_opus_context_t* params, unsigned char* packet, int size)
{
	rtp_header rtp;
	if (parse_rtp_header(packet, size, &rtp)) 
	{
		JANUS_LOG(LOG_ERR, "Parse rtp header failed \n");
		return -1;
	}

	packet += rtp.header_size;
	size -= rtp.header_size;
	params->timestamp = rtp.time;
	if (size < 0) {
		JANUS_LOG(LOG_WARN, "skipping short packet\n");
		return;
	}

	if (rtp.type != params->payload_type) {
		JANUS_LOG(LOG_ERR, "skipping packet with payload type %d\n", rtp.type);
		return;
	}

	if (rtp.seq < params->seq) {
		JANUS_LOG(LOG_ERR, "skipping out-of-sequence packet\n");
		params->seq = 0;
		return;
	}

	params->seq = rtp.seq;
	/* write the payload to our opus file */
	ogg_packet *op = op_from_pkt(packet, size);
	op->packetno = rtp.seq;
	//int  samples = opus_packet_get_nb_samples(packet, size, 48000);
	int  samples = opus_packet_get_nb_samples(packet, size, params->sample);
	if (samples > 0) params->granulepos += samples;
	op->granulepos = params->granulepos;
	ogg_stream_packetin(params->stream, op);
	free(op);

	ogg_opus_write(params);
	ogg_opus_flush(params);
}

void rtp_opus_decode_destory(struct rtp_opus_context_t* pContext)
{
	if (pContext == NULL)
	{
		return;
	}

	if (pContext->stream != NULL)
	{
		free(pContext->stream);
		pContext->stream = NULL;
	}

#ifdef TEST_DEBUG
	if (pContext->fp!=NULL)
	{
		fclose(pContext->fp);
		pContext->fp = NULL;
	}
#endif

	free(pContext);
	pContext = NULL;
	JANUS_LOG(LOG_INFO, "RTP audio decoder context destroy\n");
}

struct rtp_opus_context_t* rtp_opus_encode_init(int opus_payload_type, const char* opus_payload_name, opus_encode_packet_cb packet, void* cbdata, int opus_sample, int channel)
{
	struct rtp_opus_context_t* popus_context = NULL;
	int flag = 0;

	do
	{
		//TODO
		/*	popus_context = (struct rtp_context_t*)malloc(sizeof(struct rtp_opus_context_t));
		if (NULL == popus_context)
		{
		break;
		}

		struct rtp_payload_t vidoe_hander_cb;
		vidoe_hander_cb.alloc = NULL;
		vidoe_hander_cb.free = NULL;
		vidoe_hander_cb.packet = packet;

		popus_context->fp = fopen("/tmp/dstfile.h264", "wb");

		snprintf(popus_context->payload_name, sizeof(popus_context->payload_name), "%s", opus_payload_name);

		popus_context->opus_payload_delegate = rtp_payload_encode_create(opus_payload_type, opus_payload_name, &vidoe_hander_cb, popus_context);
		if (NULL == popus_context->opus_payload_delegate)
		{
		break;
		}
		popus_context->cbdata = cbdata;
		flag = 1;*/
	} while (0);

	if (!flag)
	{
		/*if (popus_context->opus_payload_delegate != NULL)
		{
		rtp_payload_decode_destroy(popus_context->opus_payload_delegate);
		popus_context->opus_payload_delegate = NULL;
		}

		if (popus_context != NULL)
		{
		free(popus_context);
		popus_context = NULL;
		}*/
	}
	return popus_context;
}

int rtp_opus_encode_input(struct rtp_opus_context_t* pContext, unsigned char* pbuf, int len, uint32_t timestamp)
{
	//TODO
	return 0;
}

void rtp_opus_encode_destory(struct rtp_opus_context_t* pContext)
{
	////TODO
	//if (pContext->encoder != NULL)
	//{
	//	rtp_payload_encode_destroy(pContext->encoder);
	//	pContext->encoder = NULL;
	//}

	//if (pContext != NULL)
	//{
	//	free(pContext);
	//	pContext = NULL;
	//}
}
