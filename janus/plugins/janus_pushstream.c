/*! \file   janus_pushstream.c
 * \author Lorenzo Miniero <lorenzo@meetecho.com>
 * \copyright GNU General Public License v3
 * \brief  Janus Record&Play plugin
 * \details Check the \ref pushstream for more details.
 *
 * \ingroup plugins
 * \ref plugins
 *
 * \page pushstream Record&Play plugin documentation
 * This is a simple application that implements two different
 * features: it allows you to record a message you send with WebRTC in
 * the format defined in recorded.c (MJR recording) and subsequently
 * replay this recording (or other previously recorded) through WebRTC
 * as well.
 *
 * This application aims at showing how easy recording frames sent by
 * a peer is, and how this recording can be re-used directly, without
 * necessarily involving a post-processing process (e.g., through the
 * tool we provide in janus-pp-rec.c). Notice that only audio and video
 * can be recorded and replayed in this plugin: if you're interested in
 * recording data channel messages (which Janus and the .mjr format do
 * support), you should use a different plugin instead.
 *
 * The configuration process is quite easy: just choose where the
 * recordings should be saved. The same folder will also be used to list
 * the available recordings that can be replayed.
 *
 * \note The application creates a special file in INI format with
 * \c .nfo extension for each recording that is saved. This is necessary
 * to map a specific audio .mjr file to a different video .mjr one, as
 * they always get saved in different files. If you want to replay
 * recordings you took in a different application (e.g., the streaming
 * or videoroom plugins) just copy the related files in the folder you
 * configured this plugin to use and create a .nfo file in the same
 * folder to create a mapping, e.g.:
 *
 * 		[12345678]
 * 		name = My videoroom recording
 * 		date = 2014-10-14 17:11:26
 * 		audio = mcu-audio.mjr
 * 		video = mcu-video.mjr
 *
 * \section recplayapi Record&Play API
 *
 * The Record&Play API supports several requests, some of which are
 * synchronous and some asynchronous. There are some situations, though,
 * (invalid JSON, invalid request) which will always result in a
 * synchronous error response even for asynchronous requests.
 *
 * \c list and \c update are synchronous requests, which means you'll
 * get a response directly within the context of the transaction. \c list
 * lists all the available recordings, while \c update forces the plugin
 * to scan the folder of recordings again in case some were added manually
 * and not indexed in the meanwhile.
 *
 * The \c record , \c play , \c start and \c stop requests instead are
 * all asynchronous, which means you'll get a notification about their
 * success or failure in an event. \c record asks the plugin to start
 * recording a session; \c play asks the plugin to prepare the playout
 * of one of the previously recorded sessions; \c start starts the
 * actual playout, and \c stop stops whatever the session was for, i.e.,
 * recording or replaying.
 *
 * The \c list request has to be formatted as follows:
 *
\verbatim
{
	"request" : "list"
}
\endverbatim
 *
 * A successful request will result in an array of recordings:
 *
\verbatim
{
	"pushstream" : "list",
	"list": [	// Array of recording objects
		{			// Recording #1
			"id": <numeric ID>,
			"name": "<Name of the recording>",
			"date": "<Date of the recording>",
			"audio": "<Audio rec file, if any; optional>",
			"video": "<Video rec file, if any; optional>",
			"audio_codec": "<Audio codec, if any; optional>",
			"video_codec": "<Video codec, if any; optional>"
		},
		<other recordings>
	]
}
\endverbatim
 *
 * An error instead (and the same applies to all other requests, so this
 * won't be repeated) would provide both an error code and a more verbose
 * description of the cause of the issue:
 *
\verbatim
{
	"pushstream" : "event",
	"error_code" : <numeric ID, check Macros below>,
	"error" : "<error description as a string>"
}
\endverbatim
 *
 * The \c update request instead has to be formatted as follows:
 *
\verbatim
{
	"request" : "update"
}
\endverbatim
 *
 * which will always result in an immediate ack ( \c ok ):
 *
\verbatim
{
	"pushstream" : "ok",
}
\endverbatim
 *
 * Coming to the asynchronous requests, \c record has to be attached to
 * a JSEP offer (failure to do so will result in an error) and has to be
 * formatted as follows:
 *
\verbatim
{
	"request" : "record",
	"id" : <unique numeric ID for the recording; optional, will be chosen by the server if missing>
	"name" : "<Pretty name for the recording>"
}
\endverbatim
 *
 * A successful management of this request will result in a \c recording
 * event which will include the unique ID of the recording and a JSEP
 * answer to complete the setup of the associated PeerConnection to record:
 *
\verbatim
{
	"pushstream" : "event",
	"result": {
		"status" : "recording",
		"id" : <unique numeric ID>
	}
}
\endverbatim
 *
 * A \c stop request can interrupt the recording process and tear the
 * associated PeerConnection down:
 *
\verbatim
{
	"request" : "stop",
}
\endverbatim
 *
 * This will result in a \c stopped status:
 *
\verbatim
{
	"pushstream" : "event",
	"result": {
		"status" : "stopped",
		"id" : <unique numeric ID of the interrupted recording>
	}
}
\endverbatim
 *
 * For what concerns the playout, instead, the process is slightly
 * different: you first choose a recording to replay, using \c play ,
 * and then start its playout using a \c start request. Just as before,
 * a \c stop request will interrupt the playout and tear the PeerConnection
 * down. It's very important to point out that no JSEP offer must be
 * sent for replaying a recording: in this case, it will always be the
 * plugin to generate a JSON offer (in response to a \c play request),
 * which means you'll then have to provide a JSEP answer within the
 * context of the following \c start request which will close the circle.
 *
 * A \c play request has to be formatted as follows:
 *
\verbatim
{
	"request" : "play",
	"id" : <unique numeric ID of the recording to replay>
}
\endverbatim
 *
 * This will result in a \c preparing status notification which will be
 * attached to the JSEP offer originated by the plugin in order to
 * match the media available in the recording:
 *
\verbatim
{
	"pushstream" : "event",
	"result": {
		"status" : "preparing",
		"id" : <unique numeric ID of the recording>
	}
}
\endverbatim
 *
 * A \c start request, which as anticipated must be attached to the JSEP
 * answer to the previous offer sent by the plugin, has to be formatted
 * as follows:
 *
\verbatim
{
	"request" : "start",
}
\endverbatim
 *
 * This will result in a \c playing status notification:
 *
\verbatim
{
	"pushstream" : "event",
	"result": {
		"status" : "playing"
	}
}
\endverbatim
 *
 * Just as before, a \c stop request can interrupt the playout process at
 * any time, and tear the associated PeerConnection down:
 *
\verbatim
{
	"request" : "stop",
}
\endverbatim
 *
 * This will result in a \c stopped status:
 *
\verbatim
{
	"pushstream" : "event",
	"result": {
		"status" : "stopped"
	}
}
\endverbatim
 *
 * If the plugin detects a loss of the associated PeerConnection, whether
 * as a result of a \c stop request or because the 10 seconds passed, a
 * \c done result notification is triggered to inform the application
 * the recording/playout session is over:
 *
\verbatim
{
	"pushstream" : "event",
	"result": "done"
}
\endverbatim
 */

#include "plugin.h"

#include <dirent.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <jansson.h>

#include "../debug.h"
#include "../apierror.h"
#include "../config.h"
#include "../mutex.h"
#include "../record.h"
#include "../sdp-utils.h"
#include "../rtp.h"
#include "../rtcp.h"
#include "../utils.h"
#include "../rtp_rtmp/rtp_to_opus.h"
#include "../rtp_rtmp/rtp_to_video.h"
#include "../rtp_rtmp/opus_to_pcm.h"
#include "../rtp_rtmp/aac_encode.h"
#include "../rtp_rtmp/flv_muxer_video_audio.h"
#include "../rtp_rtmp/rtmp_publish.h"

/* Plugin information */
#define JANUS_PUSHSTREAM_VERSION			4
#define JANUS_PUSHSTREAM_VERSION_STRING		"0.0.4"
#define JANUS_PUSHSTREAM_DESCRIPTION		"This is a trivial push stream plugin for Janus, to push stream with WebRTC gateway"
#define JANUS_PUSHSTREAM_NAME				"JANUS push stream plugin"
#define JANUS_PUSHSTREAM_AUTHOR				"Meetecho s.r.l."
#define JANUS_PUSHSTREAM_PACKAGE			"janus.plugin.pushstream"

/* Plugin methods */
janus_plugin *create(void);
int janus_pushstream_init(janus_callbacks *callback, const char *onfig_path);
void janus_pushstream_destroy(void);
int janus_pushstream_get_api_compatibility(void);
int janus_pushstream_get_version(void);
const char *janus_pushstream_get_version_string(void);
const char *janus_pushstream_get_description(void);
const char *janus_pushstream_get_name(void);
const char *janus_pushstream_get_author(void);
const char *janus_pushstream_get_package(void);
void janus_pushstream_create_session(janus_plugin_session *handle, int *error);
struct janus_plugin_result *janus_pushstream_handle_message(janus_plugin_session *handle, char *transaction, json_t *message, json_t *jsep);
void janus_pushstream_setup_media(janus_plugin_session *handle);
void janus_pushstream_incoming_rtp(janus_plugin_session *handle, int video, char *buf, int len);
void janus_pushstream_incoming_rtcp(janus_plugin_session *handle, int video, char *buf, int len);
void janus_pushstream_incoming_data(janus_plugin_session *handle, char *buf, int len);
void janus_pushstream_slow_link(janus_plugin_session *handle, int uplink, int video);
void janus_pushstream_hangup_media(janus_plugin_session *handle);
void janus_pushstream_destroy_session(janus_plugin_session *handle, int *error);
json_t *janus_pushstream_query_session(janus_plugin_session *handle);
static void rtp_video_packet_decode_cb(void* param, const void *packet, int bytes, uint32_t timestamp, int flags);
static void rtp_audio_packet_decode_cb(void* param, const void *packet, int bytes, uint32_t timestamp, int flags);
static void opus_to_pcm_callback(void* parame, unsigned char* pdata, int len, uint32_t timestamp);
static void aac_encode_callback(void* parame, unsigned char* pdata, int len, uint32_t timestamp);
static void flv_muxer_callback(void* flv, int type, const void* data, size_t bytes, uint32_t timestamp);

/* Plugin setup */
static janus_plugin janus_pushstream_plugin =
	JANUS_PLUGIN_INIT (
		.init = janus_pushstream_init,
		.destroy = janus_pushstream_destroy,

		.get_api_compatibility = janus_pushstream_get_api_compatibility,
		.get_version = janus_pushstream_get_version,
		.get_version_string = janus_pushstream_get_version_string,
		.get_description = janus_pushstream_get_description,
		.get_name = janus_pushstream_get_name,
		.get_author = janus_pushstream_get_author,
		.get_package = janus_pushstream_get_package,

		.create_session = janus_pushstream_create_session,
		.handle_message = janus_pushstream_handle_message,
		.setup_media = janus_pushstream_setup_media,
		.incoming_rtp = janus_pushstream_incoming_rtp,
		.incoming_rtcp = janus_pushstream_incoming_rtcp,
		.incoming_data = janus_pushstream_incoming_data,
		.slow_link = janus_pushstream_slow_link,
		.hangup_media = janus_pushstream_hangup_media,
		.destroy_session = janus_pushstream_destroy_session,
		.query_session = janus_pushstream_query_session,
	);

/* Plugin creator */
janus_plugin *create(void) {
	JANUS_LOG(LOG_VERB, "%s created!\n", JANUS_PUSHSTREAM_NAME);
	return &janus_pushstream_plugin;
}

/* Parameter validation */
static struct janus_json_parameter request_parameters[] = {
	{"request", JSON_STRING, JANUS_JSON_PARAM_REQUIRED}
};
static struct janus_json_parameter configure_parameters[] = {
	{"video-bitrate-max", JSON_INTEGER, JANUS_JSON_PARAM_POSITIVE},
	{"video-keyframe-interval", JSON_INTEGER, JANUS_JSON_PARAM_POSITIVE}
};
static struct janus_json_parameter record_parameters[] = {
	{"name", JSON_STRING, JANUS_JSON_PARAM_REQUIRED | JANUS_JSON_PARAM_NONEMPTY},
	{"id", JSON_INTEGER, JANUS_JSON_PARAM_POSITIVE},
	{"filename", JSON_STRING, 0},
	{"update", JANUS_JSON_BOOL, 0}
};
static struct janus_json_parameter play_parameters[] = {
	{"id", JSON_INTEGER, JANUS_JSON_PARAM_REQUIRED | JANUS_JSON_PARAM_POSITIVE},
	{"restart", JANUS_JSON_BOOL, 0}
};

/* Useful stuff */
static volatile gint initialized = 0, stopping = 0;
static gboolean notify_events = TRUE;
static janus_callbacks *gateway = NULL;
static GThread *handler_thread;
static void *janus_pushstream_handler(void *data);
static void janus_pushstream_hangup_media_internal(janus_plugin_session *handle);

typedef struct janus_pushstream_message {
	janus_plugin_session *handle;
	char *transaction;
	json_t *message;
	json_t *jsep;
} janus_pushstream_message;
static GAsyncQueue *messages = NULL;
static janus_pushstream_message exit_message;

typedef struct janus_pushstream_rtp_header_extension {
	uint16_t type;
	uint16_t length;
} janus_pushstream_rtp_header_extension;

typedef struct janus_pushstream_frame_packet {
	uint16_t seq;	/* RTP Sequence number */
	uint64_t ts;	/* RTP Timestamp */
	int len;		/* Length of the data */
	long offset;	/* Offset of the data in the file */
	struct janus_pushstream_frame_packet *next;
	struct janus_pushstream_frame_packet *prev;
} janus_pushstream_frame_packet;


typedef struct janus_pushstream_recording {
	guint64 id;					/* Recording unique ID */
	char *name;					/* Name of the recording */
	char *publisher;			/* addr of publisher*/
	char *date;					/* Time of the recording */
	janus_audiocodec acodec;	/* Codec used for audio, if available */
	int audio_pt;				/* Payload types to use for audio when playing recordings */
	janus_videocodec vcodec;	/* Codec used for video, if available */
	int video_pt;				/* Payload types to use for audio when playing recordings */
	char *offer;				/* The SDP offer that will be sent to watchers */
	GList *viewers;				/* List of users watching this recording */
	volatile gint completed;	/* Whether this recording was completed or still going on */
	volatile gint destroyed;	/* Whether this recording has been marked as destroyed */
	int      audio_channel;
	int      audio_sample;
	janus_refcount ref;			/* Reference counter */
	janus_mutex mutex;			/* Mutex for this recording */
} janus_pushstream_recording;
static GHashTable *recordings = NULL;
static janus_mutex recordings_mutex = JANUS_MUTEX_INITIALIZER;

typedef struct janus_pushstream_session {
	janus_plugin_session *handle;
	gint64 sdp_sessid;
	gint64 sdp_version;
	gboolean active;
	gboolean recorder;		/* Whether this session is used to record or to replay a WebRTC session */
	gboolean firefox;		/* We send Firefox users a different kind of FIR */
	janus_pushstream_recording *recording;
	janus_recorder *arc;	/* Audio recorder */
	janus_recorder *vrc;	/* Video recorder */
	janus_mutex rec_mutex;	/* Mutex to protect the recorders from race conditions */
	janus_pushstream_frame_packet *aframes;	/* Audio frames (for playout) */
	janus_pushstream_frame_packet *vframes;	/* Video frames (for playout) */
	guint video_remb_startup;
	gint64 video_remb_last;
	guint32 video_bitrate;
	guint video_keyframe_interval;			/* Keyframe request interval (ms) */
	guint64 video_keyframe_request_last;	/* Timestamp of last keyframe request sent */
	gint video_fir_seq;
	janus_rtp_switching_context context;
	uint32_t ssrc[3];		/* Only needed in case VP8 (or H.264) simulcasting is involved */
	janus_rtp_simulcasting_context sim_context;
	janus_vp8_simulcast_context vp8_context;
	volatile gint hangingup;
	volatile gint destroyed;
	struct rtp_video_context_t* video_ctx;
	struct rtp_opus_context_t* audio_ctx;
	struct opus_to_pcm_context_t * opus_to_pcm_ctx;
	struct aac_encode_context_t*  aac_encode_ctx;
	struct flv_muxer_context_t* flv_muxer_ctx;
	struct rtmp_client_publish_context_t* rtmp_client_ctx;
	janus_refcount ref;
} janus_pushstream_session;
static GHashTable *sessions;
static janus_mutex sessions_mutex = JANUS_MUTEX_INITIALIZER;

static void janus_pushstream_session_destroy(janus_pushstream_session *session) {
	if(session && g_atomic_int_compare_and_exchange(&session->destroyed, 0, 1))
		janus_refcount_decrease(&session->ref);
}

static void janus_pushstream_session_free(const janus_refcount *session_ref) {
	janus_pushstream_session *session = janus_refcount_containerof(session_ref, janus_pushstream_session, ref);
	/* Remove the reference to the core plugin session */
	janus_refcount_decrease(&session->handle->ref);
	/* This session can be destroyed, free all the resources */
	g_free(session);
}


static void janus_pushstream_recording_destroy(janus_pushstream_recording *recording) {
	if(recording && g_atomic_int_compare_and_exchange(&recording->destroyed, 0, 1))
		janus_refcount_decrease(&recording->ref);
}

static void janus_pushstream_recording_free(const janus_refcount *recording_ref) {
	janus_pushstream_recording *recording = janus_refcount_containerof(recording_ref, janus_pushstream_recording, ref);
	/* This recording can be destroyed, free all the resources */
	g_free(recording->name);
	g_free(recording->date);
	g_free(recording->offer);
	g_free(recording->publisher);
	g_free(recording);
}


static char *recordings_path = NULL;
void janus_pushstream_update_recordings_list(void);
static void *janus_pushstream_playout_thread(void *data);

/* Helper to send RTCP feedback back to recorders, if needed */
void janus_pushstream_send_rtcp_feedback(janus_plugin_session *handle, int video, char *buf, int len);

/* To make things easier, we use static payload types for viewers (unless it's for G.711 or G.722) */
#define AUDIO_PT		111
#define VIDEO_PT		100

/* Helper method to check which codec was used in a specific recording */
static const char *janus_pushstream_parse_codec(const char *dir, const char *filename) {
	if(dir == NULL || filename == NULL)
		return NULL;
	char source[1024];
	if(strstr(filename, ".mjr"))
		g_snprintf(source, 1024, "%s/%s", dir, filename);
	else
		g_snprintf(source, 1024, "%s/%s.mjr", dir, filename);
	FILE *file = fopen(source, "rb");
	if(file == NULL) {
		JANUS_LOG(LOG_ERR, "Could not open file %s\n", source);
		return NULL;
	}
	fseek(file, 0L, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0L, SEEK_SET);

	/* Pre-parse */
	JANUS_LOG(LOG_VERB, "Pre-parsing file %s to generate ordered index...\n", source);
	gboolean parsed_header = FALSE;
	int bytes = 0;
	long offset = 0;
	uint16_t len = 0;
	char prebuffer[1500];
	memset(prebuffer, 0, 1500);
	/* Let's look for timestamp resets first */
	while(offset < fsize) {
		/* Read frame header */
		fseek(file, offset, SEEK_SET);
		bytes = fread(prebuffer, sizeof(char), 8, file);
		if(bytes != 8 || prebuffer[0] != 'M') {
			JANUS_LOG(LOG_ERR, "Invalid header...\n");
			fclose(file);
			return NULL;
		}
		if(prebuffer[1] == 'E') {
			/* Either the old .mjr format header ('MEETECHO' header followed by 'audio' or 'video'), or a frame */
			offset += 8;
			bytes = fread(&len, sizeof(uint16_t), 1, file);
			len = ntohs(len);
			offset += 2;
			if(len == 5 && !parsed_header) {
				/* This is the main header */
				parsed_header = TRUE;
				bytes = fread(prebuffer, sizeof(char), 5, file);
				if(prebuffer[0] == 'v') {
					JANUS_LOG(LOG_VERB, "This is an old video recording, assuming VP8\n");
					fclose(file);
					return "vp8";
				} else if(prebuffer[0] == 'a') {
					JANUS_LOG(LOG_VERB, "This is an old audio recording, assuming Opus\n");
					fclose(file);
					return "opus";
				}
			}
			JANUS_LOG(LOG_WARN, "Unsupported recording media type...\n");
			fclose(file);
			return NULL;
		} else if(prebuffer[1] == 'J') {
			/* New .mjr format */
			offset += 8;
			bytes = fread(&len, sizeof(uint16_t), 1, file);
			len = ntohs(len);
			offset += 2;
			if(len > 0 && !parsed_header) {
				/* This is the info header */
				bytes = fread(prebuffer, sizeof(char), len, file);
				if(bytes < 0) {
					JANUS_LOG(LOG_ERR, "Error reading from file... %s\n", strerror(errno));
					fclose(file);
					return NULL;
				}
				parsed_header = TRUE;
				prebuffer[len] = '\0';
				json_error_t error;
				json_t *info = json_loads(prebuffer, 0, &error);
				if(!info) {
					JANUS_LOG(LOG_ERR, "JSON error: on line %d: %s\n", error.line, error.text);
					JANUS_LOG(LOG_WARN, "Error parsing info header...\n");
					fclose(file);
					return NULL;
				}
				/* Is it audio or video? */
				json_t *type = json_object_get(info, "t");
				if(!type || !json_is_string(type)) {
					JANUS_LOG(LOG_WARN, "Missing/invalid recording type in info header...\n");
					json_decref(info);
					fclose(file);
					return NULL;
				}
				const char *t = json_string_value(type);
				gboolean video = FALSE;
				if(!strcasecmp(t, "v")) {
					video = TRUE;
				} else if(!strcasecmp(t, "a")) {
					video = FALSE;
				} else {
					JANUS_LOG(LOG_WARN, "Unsupported recording type '%s' in info header...\n", t);
					json_decref(info);
					fclose(file);
					return NULL;
				}
				/* What codec was used? */
				json_t *codec = json_object_get(info, "c");
				if(!codec || !json_is_string(codec)) {
					JANUS_LOG(LOG_WARN, "Missing recording codec in info header...\n");
					json_decref(info);
					fclose(file);
					return NULL;
				}
				const char *c = json_string_value(codec);
				const char *mcodec = janus_sdp_match_preferred_codec(video ? JANUS_SDP_VIDEO : JANUS_SDP_AUDIO, (char *)c);
				if(mcodec != NULL) {
					/* Found! */
					json_decref(info);
					fclose(file);
					return mcodec;
				}
				json_decref(info);
			}
			JANUS_LOG(LOG_WARN, "No codec found...\n");
			fclose(file);
			return NULL;
		} else {
			JANUS_LOG(LOG_ERR, "Invalid header...\n");
			fclose(file);
			return NULL;
		}
	}
	fclose(file);
	return NULL;
}

/* Helper method to prepare an SDP offer when a recording is available */
static int janus_pushstream_generate_offer(janus_pushstream_recording *rec) {
	if(rec == NULL)
		return -1;
	/* Prepare an SDP offer we'll send to playout viewers */
	gboolean offer_audio = rec->acodec != JANUS_AUDIOCODEC_NONE,
		offer_video =  rec->vcodec != JANUS_VIDEOCODEC_NONE;
	char s_name[100];
	g_snprintf(s_name, sizeof(s_name), "Recording %"SCNu64, rec->id);
	janus_sdp *offer = janus_sdp_generate_offer(
		s_name, "1.1.1.1",
		JANUS_SDP_OA_AUDIO, offer_audio,
		JANUS_SDP_OA_AUDIO_CODEC, janus_audiocodec_name(rec->acodec),
		JANUS_SDP_OA_AUDIO_PT, rec->audio_pt,
		JANUS_SDP_OA_AUDIO_DIRECTION, JANUS_SDP_SENDONLY,
		JANUS_SDP_OA_VIDEO, offer_video,
		JANUS_SDP_OA_VIDEO_CODEC, janus_videocodec_name(rec->vcodec),
		JANUS_SDP_OA_VIDEO_PT, rec->video_pt,
		JANUS_SDP_OA_VIDEO_DIRECTION, JANUS_SDP_SENDONLY,
		JANUS_SDP_OA_DATA, FALSE,
		JANUS_SDP_OA_DONE);
	g_free(rec->offer);
	rec->offer = janus_sdp_write(offer);
	janus_sdp_destroy(offer);
	return 0;
}

static void janus_pushstream_message_free(janus_pushstream_message *msg) {
	if(!msg || msg == &exit_message)
		return;

	if(msg->handle && msg->handle->plugin_handle) {
		janus_pushstream_session *session = (janus_pushstream_session *)msg->handle->plugin_handle;
		janus_refcount_decrease(&session->ref);
	}
	msg->handle = NULL;

	g_free(msg->transaction);
	msg->transaction = NULL;
	if(msg->message)
		json_decref(msg->message);
	msg->message = NULL;
	if(msg->jsep)
		json_decref(msg->jsep);
	msg->jsep = NULL;

	g_free(msg);
}


/* Error codes */
#define JANUS_PUSHSTREAM_ERROR_NO_MESSAGE			411
#define JANUS_PUSHSTREAM_ERROR_INVALID_JSON			412
#define JANUS_PUSHSTREAM_ERROR_INVALID_REQUEST		413
#define JANUS_PUSHSTREAM_ERROR_INVALID_ELEMENT		414
#define JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT		415
#define JANUS_PUSHSTREAM_ERROR_NOT_FOUND			416
#define JANUS_PUSHSTREAM_ERROR_INVALID_RECORDING	417
#define JANUS_PUSHSTREAM_ERROR_INVALID_STATE		418
#define JANUS_PUSHSTREAM_ERROR_INVALID_SDP			419
#define JANUS_PUSHSTREAM_ERROR_RECORDING_EXISTS		420
#define JANUS_PUSHSTREAM_ERROR_UNKNOWN_ERROR		499
#define JANUS_PUSHSTREAM_ERROR_CREATE_RTMP_CLIETN_FAILED 500
#define JANUS_PUSHSTREAM_ERROR_CREATE_FLV_MUXER_FAILED 501
#define JANUS_PUSHSTREAM_ERROR_CREATE_AAC_ENCODER_FAILED 502
#define JANUS_PUSHSTREAM_ERROR_CREATE_OPUS_DECODER_FAILED 503
#define JANUS_PUSHSTREAM_ERROR_CREATE_RTP_VIDOE_DECODER_FAILED 504
#define JANUS_PUSHSTREAM_ERROR_CREATE_RTP_AUDIO_DECODER_FAILED 505




/* Plugin implementation */
int janus_pushstream_init(janus_callbacks *callback, const char *config_path) {
	if(g_atomic_int_get(&stopping)) {
		/* Still stopping from before */
		return -1;
	}
	if(callback == NULL || config_path == NULL) {
		/* Invalid arguments */
		return -1;
	}

	/* Read configuration */
	char filename[255];
	g_snprintf(filename, 255, "%s/%s.cfg", config_path, JANUS_PUSHSTREAM_PACKAGE);
	JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
	janus_config *config = janus_config_parse(filename);
	if(config != NULL)
		janus_config_print(config);
	/* Parse configuration */
	if(config != NULL) {
		janus_config_item *path = janus_config_get_item_drilldown(config, "general", "path");
		if(path && path->value)
			recordings_path = g_strdup(path->value);
		janus_config_item *events = janus_config_get_item_drilldown(config, "general", "events");
		if(events != NULL && events->value != NULL)
			notify_events = janus_is_true(events->value);
		if(!notify_events && callback->events_is_enabled()) {
			JANUS_LOG(LOG_WARN, "Notification of events to handlers disabled for %s\n", JANUS_PUSHSTREAM_NAME);
		}
		/* Done */
		janus_config_destroy(config);
		config = NULL;
	}
	if(recordings_path == NULL) {
		JANUS_LOG(LOG_FATAL, "No recordings path specified, giving up...\n");
		return -1;
	}
	/* Create the folder, if needed */
	struct stat st = {0};
	if(stat(recordings_path, &st) == -1) {
		int res = janus_mkdir(recordings_path, 0755);
		JANUS_LOG(LOG_VERB, "Creating folder: %d\n", res);
		if(res != 0) {
			JANUS_LOG(LOG_ERR, "%s", strerror(errno));
			return -1;	/* No point going on... */
		}
	}
	recordings = g_hash_table_new_full(g_int64_hash, g_int64_equal, (GDestroyNotify)g_free, (GDestroyNotify)janus_pushstream_recording_destroy);

	sessions = g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)janus_pushstream_session_destroy);
	messages = g_async_queue_new_full((GDestroyNotify) janus_pushstream_message_free);
	/* This is the callback we'll need to invoke to contact the Janus core */
	gateway = callback;

	g_atomic_int_set(&initialized, 1);

	/* Launch the thread that will handle incoming messages */
	GError *error = NULL;
	handler_thread = g_thread_try_new("pushstream handler", janus_pushstream_handler, NULL, &error);
	if(error != NULL) {
		g_atomic_int_set(&initialized, 0);
		JANUS_LOG(LOG_ERR, "Got error %d (%s) trying to launch the Record&Play handler thread...\n", error->code, error->message ? error->message : "??");
		return -1;
	}
	JANUS_LOG(LOG_INFO, "%s initialized!\n", JANUS_PUSHSTREAM_NAME);
	return 0;
}

void janus_pushstream_destroy(void) {
	if(!g_atomic_int_get(&initialized))
		return;
	g_atomic_int_set(&stopping, 1);

	g_async_queue_push(messages, &exit_message);
	if(handler_thread != NULL) {
		g_thread_join(handler_thread);
		handler_thread = NULL;
	}
	/* FIXME We should destroy the sessions cleanly */
	janus_mutex_lock(&sessions_mutex);
	g_hash_table_destroy(sessions);
	sessions = NULL;
	g_hash_table_destroy(recordings);
	recordings = NULL;
	janus_mutex_unlock(&sessions_mutex);
	g_async_queue_unref(messages);
	messages = NULL;
	g_atomic_int_set(&initialized, 0);
	g_atomic_int_set(&stopping, 0);
	JANUS_LOG(LOG_INFO, "%s destroyed!\n", JANUS_PUSHSTREAM_NAME);
}

int janus_pushstream_get_api_compatibility(void) {
	/* Important! This is what your plugin MUST always return: don't lie here or bad things will happen */
	return JANUS_PLUGIN_API_VERSION;
}

int janus_pushstream_get_version(void) {
	return JANUS_PUSHSTREAM_VERSION;
}

const char *janus_pushstream_get_version_string(void) {
	return JANUS_PUSHSTREAM_VERSION_STRING;
}

const char *janus_pushstream_get_description(void) {
	return JANUS_PUSHSTREAM_DESCRIPTION;
}

const char *janus_pushstream_get_name(void) {
	return JANUS_PUSHSTREAM_NAME;
}

const char *janus_pushstream_get_author(void) {
	return JANUS_PUSHSTREAM_AUTHOR;
}

const char *janus_pushstream_get_package(void) {
	return JANUS_PUSHSTREAM_PACKAGE;
}

static janus_pushstream_session *janus_pushstream_lookup_session(janus_plugin_session *handle) {
	janus_pushstream_session *session = NULL;
	if (g_hash_table_contains(sessions, handle)) {
		session = (janus_pushstream_session *)handle->plugin_handle;
	}
	return session;
}

void janus_pushstream_create_session(janus_plugin_session *handle, int *error) {
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized)) {
		*error = -1;
		return;
	}
	janus_pushstream_session *session = g_malloc0(sizeof(janus_pushstream_session));
	session->handle = handle;
	session->active = FALSE;
	session->recorder = FALSE;
	session->firefox = FALSE;
	session->arc = NULL;
	session->vrc = NULL;
	janus_mutex_init(&session->rec_mutex);
	g_atomic_int_set(&session->hangingup, 0);
	g_atomic_int_set(&session->destroyed, 0);
	session->video_remb_startup = 4;
	session->video_remb_last = janus_get_monotonic_time();
	session->video_bitrate = 1024 * 1024; 		/* This is 1mbps by default */
	session->video_keyframe_request_last = 0;
	session->video_keyframe_interval = 4000; 	/* 4 seconds by default */
	session->video_fir_seq = 0;
	janus_rtp_switching_context_reset(&session->context);
	janus_rtp_simulcasting_context_reset(&session->sim_context);
	janus_vp8_simulcast_context_reset(&session->vp8_context);
	janus_refcount_init(&session->ref, janus_pushstream_session_free);
	handle->plugin_handle = session;

	janus_mutex_lock(&sessions_mutex);
	g_hash_table_insert(sessions, handle, session);
	janus_mutex_unlock(&sessions_mutex);

	return;
}

void janus_pushstream_destroy_session(janus_plugin_session *handle, int *error) {
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized)) {
		*error = -1;
		return;
	}
	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session) {
		janus_mutex_unlock(&sessions_mutex);
		JANUS_LOG(LOG_ERR, "No Record&Play session associated with this handle...\n");
		*error = -2;
		return;
	}
	JANUS_LOG(LOG_VERB, "Removing Record&Play session...\n");
	janus_pushstream_hangup_media_internal(handle);
	if (session->video_ctx!=NULL)
	{
		rtp_Video_decode_destory(session->video_ctx);
		session->video_ctx = NULL;
	}
	if (session->audio_ctx!=NULL)
	{
		rtp_opus_decode_destory(session->audio_ctx);
		session->audio_ctx = NULL;
	}
	if (session->opus_to_pcm_ctx!=NULL)
	{
		opus_to_pcm_destory(session->opus_to_pcm_ctx);
		session->opus_to_pcm_ctx = NULL;
	}
	if (session->aac_encode_ctx!=NULL)
	{
		aac_encode_destory(session->aac_encode_ctx);
		session->aac_encode_ctx = NULL;
	}
	if (session->flv_muxer_ctx !=NULL)
	{
		flv_muxer_video_audio_destory(session->flv_muxer_ctx);
		session->flv_muxer_ctx = NULL;
	}
	if (session->rtmp_client_ctx !=NULL)
	{
		rtmp_client_context_destroy(session->rtmp_client_ctx);
		session->rtmp_client_ctx = NULL;
	}
	g_hash_table_remove(sessions, handle);
	janus_mutex_unlock(&sessions_mutex);
	return;
}

json_t *janus_pushstream_query_session(janus_plugin_session *handle) {
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized)) {
		return NULL;
	}
	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session) {
		janus_mutex_unlock(&sessions_mutex);
		JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
		return NULL;
	}
	janus_refcount_increase(&session->ref);
	janus_mutex_unlock(&sessions_mutex);
	/* In the echo test, every session is the same: we just provide some configure info */
	json_t *info = json_object();
	json_object_set_new(info, "type", json_string(session->recorder ? "recorder" : (session->recording ? "player" : "none")));
	if(session->recording) {
		janus_refcount_increase(&session->recording->ref);
		json_object_set_new(info, "recording_id", json_integer(session->recording->id));
		json_object_set_new(info, "recording_name", json_string(session->recording->name));
		janus_refcount_decrease(&session->recording->ref);
	}
	json_object_set_new(info, "hangingup", json_integer(g_atomic_int_get(&session->hangingup)));
	json_object_set_new(info, "destroyed", json_integer(g_atomic_int_get(&session->destroyed)));
	janus_refcount_decrease(&session->ref);
	return info;
}

struct janus_plugin_result *janus_pushstream_handle_message(janus_plugin_session *handle, char *transaction, json_t *message, json_t *jsep) {
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return janus_plugin_result_new(JANUS_PLUGIN_ERROR, g_atomic_int_get(&stopping) ? "Shutting down" : "Plugin not initialized", NULL);

	/* Pre-parse the message */
	int error_code = 0;
	char error_cause[512];
	json_t *root = message;
	json_t *response = NULL;

	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session) {
		janus_mutex_unlock(&sessions_mutex);
		JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
		error_code = JANUS_PUSHSTREAM_ERROR_UNKNOWN_ERROR;
		g_snprintf(error_cause, 512, "%s", "No session associated with this handle...");
		goto plugin_response;
	}
	/* Increase the reference counter for this session: we'll decrease it after we handle the message */
	janus_refcount_increase(&session->ref);
	janus_mutex_unlock(&sessions_mutex);
	if(g_atomic_int_get(&session->destroyed)) {
		JANUS_LOG(LOG_ERR, "Session has already been destroyed...\n");
		error_code = JANUS_PUSHSTREAM_ERROR_UNKNOWN_ERROR;
		g_snprintf(error_cause, 512, "%s", "Session has already been destroyed...");
		goto plugin_response;
	}

	if(message == NULL) {
		JANUS_LOG(LOG_ERR, "No message??\n");
		error_code = JANUS_PUSHSTREAM_ERROR_NO_MESSAGE;
		g_snprintf(error_cause, 512, "%s", "No message??");
		goto plugin_response;
	}
	if(!json_is_object(root)) {
		JANUS_LOG(LOG_ERR, "JSON error: not an object\n");
		error_code = JANUS_PUSHSTREAM_ERROR_INVALID_JSON;
		g_snprintf(error_cause, 512, "JSON error: not an object");
		goto plugin_response;
	}
	/* Get the request first */
	JANUS_VALIDATE_JSON_OBJECT(root, request_parameters,
		error_code, error_cause, TRUE,
		JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT, JANUS_PUSHSTREAM_ERROR_INVALID_ELEMENT);
	if(error_code != 0)
		goto plugin_response;
	json_t *request = json_object_get(root, "request");
	/* Some requests ('create' and 'destroy') can be handled synchronously */
	const char *request_text = json_string_value(request);
	if(!strcasecmp(request_text, "configure")) {
		JANUS_VALIDATE_JSON_OBJECT(root, configure_parameters,
			error_code, error_cause, TRUE,
			JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT, JANUS_PUSHSTREAM_ERROR_INVALID_ELEMENT);
		if(error_code != 0)
			goto plugin_response;
		json_t *video_bitrate_max = json_object_get(root, "video-bitrate-max");
		if(video_bitrate_max) {
			session->video_bitrate = json_integer_value(video_bitrate_max);
			JANUS_LOG(LOG_VERB, "Video bitrate has been set to %"SCNu32"\n", session->video_bitrate);
		}
		json_t *video_keyframe_interval= json_object_get(root, "video-keyframe-interval");
		if(video_keyframe_interval) {
			session->video_keyframe_interval = json_integer_value(video_keyframe_interval);
			JANUS_LOG(LOG_VERB, "Video keyframe interval has been set to %u\n", session->video_keyframe_interval);
			if (session->video_keyframe_interval < 1000) {
				session->video_keyframe_interval = 1000;
				JANUS_LOG(LOG_WARN, "Video keyframe interval has been force reset to %u\n", session->video_keyframe_interval);
			}
		}
		response = json_object();
		json_object_set_new(response, "pushstream", json_string("configure"));
		json_object_set_new(response, "status", json_string("ok"));
		/* Return a success, and also let the client be aware of what changed, to allow crosschecks */
		json_t *settings = json_object();
		json_object_set_new(settings, "video-keyframe-interval", json_integer(session->video_keyframe_interval));
		json_object_set_new(settings, "video-bitrate-max", json_integer(session->video_bitrate));
		json_object_set_new(response, "settings", settings);
		goto plugin_response;
	} else if(!strcasecmp(request_text, "record") || !strcasecmp(request_text, "play")
			|| !strcasecmp(request_text, "start") || !strcasecmp(request_text, "stop")) {
		/* These messages are handled asynchronously */
		janus_pushstream_message *msg = g_malloc(sizeof(janus_pushstream_message));
		msg->handle = handle;
		msg->transaction = transaction;
		msg->message = root;
		msg->jsep = jsep;

		g_async_queue_push(messages, msg);

		return janus_plugin_result_new(JANUS_PLUGIN_OK_WAIT, NULL, NULL);
	} else {
		JANUS_LOG(LOG_VERB, "Unknown request '%s'\n", request_text);
		error_code = JANUS_PUSHSTREAM_ERROR_INVALID_REQUEST;
		g_snprintf(error_cause, 512, "Unknown request '%s'", request_text);
	}

plugin_response:
		{
			if(error_code == 0 && !response) {
				error_code = JANUS_PUSHSTREAM_ERROR_UNKNOWN_ERROR;
				g_snprintf(error_cause, 512, "Invalid response");
			}
			if(error_code != 0) {
				/* Prepare JSON error event */
				json_t *event = json_object();
				json_object_set_new(event, "pushstream", json_string("event"));
				json_object_set_new(event, "error_code", json_integer(error_code));
				json_object_set_new(event, "error", json_string(error_cause));
				response = event;
			}
			if(root != NULL)
				json_decref(root);
			if(jsep != NULL)
				json_decref(jsep);
			g_free(transaction);

			if(session != NULL)
				janus_refcount_decrease(&session->ref);
			return janus_plugin_result_new(JANUS_PLUGIN_OK, NULL, response);
		}

}

void janus_pushstream_setup_media(janus_plugin_session *handle) {
	JANUS_LOG(LOG_INFO, "[%s-%p] WebRTC media is now available\n", JANUS_PUSHSTREAM_PACKAGE, handle);
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return;
	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session) {
		janus_mutex_unlock(&sessions_mutex);
		JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
		return;
	}
	if(g_atomic_int_get(&session->destroyed)) {
		janus_mutex_unlock(&sessions_mutex);
		return;
	}
	janus_refcount_increase(&session->ref);
	janus_mutex_unlock(&sessions_mutex);
	g_atomic_int_set(&session->hangingup, 0);
	/* Take note of the fact that the session is now active */
	session->active = TRUE;
	janus_refcount_decrease(&session->ref);
}

void janus_pushstream_send_rtcp_feedback(janus_plugin_session *handle, int video, char *buf, int len) {
	if(video != 1)
		return;	/* We just do this for video, for now */

	janus_pushstream_session *session = (janus_pushstream_session *)handle->plugin_handle;
	char rtcpbuf[24];

	/* Send a RR+SDES+REMB every five seconds, or ASAP while we are still
	 * ramping up (first 4 RTP packets) */
	gint64 now = janus_get_monotonic_time();
	gint64 elapsed = now - session->video_remb_last;
	gboolean remb_rampup = session->video_remb_startup > 0;

	if(remb_rampup || (elapsed >= 5*G_USEC_PER_SEC)) {
		guint32 bitrate = session->video_bitrate;

		if(remb_rampup) {
			bitrate = bitrate / session->video_remb_startup;
			session->video_remb_startup--;
		}

		/* Send a new REMB back */
		char rtcpbuf[24];
		janus_rtcp_remb((char *)(&rtcpbuf), 24, bitrate);
		gateway->relay_rtcp(handle, video, rtcpbuf, 24);

		session->video_remb_last = now;
	}

	/* Request a keyframe on a regular basis (every session->video_keyframe_interval ms) */
	elapsed = now - session->video_keyframe_request_last;
	gint64 interval = (gint64)(session->video_keyframe_interval / 1000) * G_USEC_PER_SEC;

	if(elapsed >= interval) {
		/* Send both a FIR and a PLI, just to be sure */
		janus_rtcp_fir((char *)&rtcpbuf, 20, &session->video_fir_seq);
		gateway->relay_rtcp(handle, video, rtcpbuf, 20);
		janus_rtcp_pli((char *)&rtcpbuf, 12);
		gateway->relay_rtcp(handle, video, rtcpbuf, 12);
		session->video_keyframe_request_last = now;
	}
}

void janus_pushstream_incoming_rtp(janus_plugin_session *handle, int video, char *buf, int len) {
	if(handle == NULL || g_atomic_int_get(&handle->stopped) || g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return;
	janus_pushstream_session *session = (janus_pushstream_session *)handle->plugin_handle;
	if(!session) {
		JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
		return;
	}
	if(g_atomic_int_get(&session->destroyed))
		return;
	if(!session->recorder || !session->recording)
		return;
	if(video && session->ssrc[0] != 0) {
		/* Handle simulcast: backup the header information first */
		janus_rtp_header *header = (janus_rtp_header *)buf;
		uint32_t seq_number = ntohs(header->seq_number);
		uint32_t timestamp = ntohl(header->timestamp);
		uint32_t ssrc = ntohl(header->ssrc);
		/* Process this packet: don't save if it's not the SSRC/layer we wanted to handle */
		gboolean save = janus_rtp_simulcasting_context_process_rtp(&session->sim_context,
			buf, len, session->ssrc, session->recording->vcodec, &session->context);
		/* Do we need to drop this? */
		if(!save)
			return;
		if(session->sim_context.need_pli) {
			/* Send a PLI */
			JANUS_LOG(LOG_VERB, "We need a PLI for the simulcast context\n");
			char rtcpbuf[12];
			memset(rtcpbuf, 0, 12);
			janus_rtcp_pli((char *)&rtcpbuf, 12);
			gateway->relay_rtcp(handle, 1, rtcpbuf, 12);
		}
		/* If we got here, update the RTP header and save the packet */
		janus_rtp_header_update(header, &session->context, TRUE, 0);
		if(session->recording->vcodec == JANUS_VIDEOCODEC_VP8) {
			int plen = 0;
			char *payload = janus_rtp_payload(buf, len, &plen);
			janus_vp8_simulcast_descriptor_update(payload, plen, &session->vp8_context, session->sim_context.changed_substream);
		}
		/* Save the frame if we're recording (and make sure the SSRC never changes even if the substream does) */
		header->ssrc = htonl(session->ssrc[0]);
		//janus_recorder_save_frame(session->vrc, buf, len);
		
		rtp_video_decode_input(session->video_ctx, buf, len);
		
		/* Restore header or core statistics will be messed up */
		header->ssrc = htonl(ssrc);
		header->timestamp = htonl(timestamp);
		header->seq_number = htons(seq_number);
	} else {
		/* Save the frame if we're recording */
		//janus_recorder_save_frame(video ? session->vrc : session->arc, buf, len);
		if (video)
		{
			rtp_video_decode_input(session->video_ctx, buf, len);
		}
		else
		{
			rtp_opus_decode_input(session->audio_ctx, buf, len);
		}
	}

	janus_pushstream_send_rtcp_feedback(handle, video, buf, len);
}

void janus_pushstream_incoming_rtcp(janus_plugin_session *handle, int video, char *buf, int len) {
	if(handle == NULL || g_atomic_int_get(&handle->stopped) || g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return;
}

void janus_pushstream_incoming_data(janus_plugin_session *handle, char *buf, int len) {
	if(handle == NULL || g_atomic_int_get(&handle->stopped) || g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return;
	/* FIXME We don't care */
}

void janus_pushstream_slow_link(janus_plugin_session *handle, int uplink, int video) {
	if(handle == NULL || g_atomic_int_get(&handle->stopped) || g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized) || !gateway)
		return;

	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session || g_atomic_int_get(&session->destroyed)) {
		janus_mutex_unlock(&sessions_mutex);
		return;
	}
	janus_refcount_increase(&session->ref);
	janus_mutex_unlock(&sessions_mutex);

	json_t *event = json_object();
	json_object_set_new(event, "pushstream", json_string("event"));
	json_t *result = json_object();
	json_object_set_new(result, "status", json_string("slow_link"));
	/* What is uplink for the server is downlink for the client, so turn the tables */
	json_object_set_new(result, "current-bitrate", json_integer(session->video_bitrate));
	json_object_set_new(result, "uplink", json_integer(uplink ? 0 : 1));
	json_object_set_new(event, "result", result);
	gateway->push_event(session->handle, &janus_pushstream_plugin, NULL, event, NULL);
	json_decref(event);
	janus_refcount_decrease(&session->ref);
}

void janus_pushstream_hangup_media(janus_plugin_session *handle) {
	JANUS_LOG(LOG_INFO, "[%s-%p] No WebRTC media anymore\n", JANUS_PUSHSTREAM_PACKAGE, handle);
	janus_mutex_lock(&sessions_mutex);
	janus_pushstream_hangup_media_internal(handle);
	janus_mutex_unlock(&sessions_mutex);
}

static void janus_pushstream_hangup_media_internal(janus_plugin_session *handle) {
	if(g_atomic_int_get(&stopping) || !g_atomic_int_get(&initialized))
		return;
	janus_pushstream_session *session = janus_pushstream_lookup_session(handle);
	if(!session) {
		JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
		return;
	}
	session->active = FALSE;
	if(g_atomic_int_get(&session->destroyed))
		return;
	if(!g_atomic_int_compare_and_exchange(&session->hangingup, 0, 1))
		return;
	janus_rtp_switching_context_reset(&session->context);
	janus_rtp_simulcasting_context_reset(&session->sim_context);
	janus_vp8_simulcast_context_reset(&session->vp8_context);

	/* Send an event to the browser and tell it's over */
	json_t *event = json_object();
	json_object_set_new(event, "pushstream", json_string("event"));
	json_object_set_new(event, "result", json_string("done"));
	int ret = gateway->push_event(handle, &janus_pushstream_plugin, NULL, event, NULL);
	JANUS_LOG(LOG_VERB, "  >> Pushing event: %d (%s)\n", ret, janus_get_api_error(ret));
	json_decref(event);

	session->active = FALSE;
	janus_mutex_lock(&session->rec_mutex);
	if(session->arc) {
		janus_recorder *rc = session->arc;
		session->arc = NULL;
		janus_recorder_close(rc);
		JANUS_LOG(LOG_INFO, "Closed audio recording %s\n", rc->filename ? rc->filename : "??");
		janus_recorder_destroy(rc);
	}
	if(session->vrc) {
		janus_recorder *rc = session->vrc;
		session->vrc = NULL;
		janus_recorder_close(rc);
		JANUS_LOG(LOG_INFO, "Closed video recording %s\n", rc->filename ? rc->filename : "??");
		janus_recorder_destroy(rc);
	}
	janus_mutex_unlock(&session->rec_mutex);
	if(session->recorder) {
		if(session->recording) {
			if (janus_pushstream_generate_offer(session->recording) < 0) {
				JANUS_LOG(LOG_WARN, "Could not generate offer for recording %"SCNu64"...\n", session->recording->id);
			}
		} else {
			JANUS_LOG(LOG_WARN, "Got a stop but missing recorder/recording! .nfo file may not have been generated...\n");
		}
	}
	if(session->recording) {
		janus_refcount_decrease(&session->recording->ref);
		session->recording = NULL;
	}
	g_atomic_int_set(&session->hangingup, 0);
}

/* Thread to handle incoming messages */
static void *janus_pushstream_handler(void *data) {
	JANUS_LOG(LOG_VERB, "Joining Record&Play handler thread\n");
	janus_pushstream_message *msg = NULL;
	int error_code = 0;
	char error_cause[512];
	json_t *root = NULL;
	while(g_atomic_int_get(&initialized) && !g_atomic_int_get(&stopping)) {
		msg = g_async_queue_pop(messages);
		if(msg == &exit_message)
			break;
		if(msg->handle == NULL) {
			janus_pushstream_message_free(msg);
			continue;
		}
		janus_mutex_lock(&sessions_mutex);
		janus_pushstream_session *session = janus_pushstream_lookup_session(msg->handle);
		if(!session) {
			janus_mutex_unlock(&sessions_mutex);
			JANUS_LOG(LOG_ERR, "No session associated with this handle...\n");
			janus_pushstream_message_free(msg);
			continue;
		}
		if(g_atomic_int_get(&session->destroyed)) {
			janus_mutex_unlock(&sessions_mutex);
			janus_pushstream_message_free(msg);
			continue;
		}
		janus_mutex_unlock(&sessions_mutex);
		/* Handle request */
		error_code = 0;
		root = NULL;
		if(msg->message == NULL) {
			JANUS_LOG(LOG_ERR, "No message??\n");
			error_code = JANUS_PUSHSTREAM_ERROR_NO_MESSAGE;
			g_snprintf(error_cause, 512, "%s", "No message??");
			goto error;
		}
		root = msg->message;
		/* Get the request first */
		JANUS_VALIDATE_JSON_OBJECT(root, request_parameters,
			error_code, error_cause, TRUE,
			JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT, JANUS_PUSHSTREAM_ERROR_INVALID_ELEMENT);
		if(error_code != 0)
			goto error;
		const char *msg_sdp_type = json_string_value(json_object_get(msg->jsep, "type"));
		const char *msg_sdp = json_string_value(json_object_get(msg->jsep, "sdp"));
		json_t *request = json_object_get(root, "request");
		const char *request_text = json_string_value(request);
		json_t *event = NULL;
		json_t *result = NULL;
		char *sdp = NULL;
		gboolean sdp_update = FALSE;
		if(json_object_get(msg->jsep, "update") != NULL)
			sdp_update = json_is_true(json_object_get(msg->jsep, "update"));
		const char *filename_text = NULL;
		if(!strcasecmp(request_text, "record")) {
			if(!msg_sdp || !msg_sdp_type || strcasecmp(msg_sdp_type, "offer")) {
				JANUS_LOG(LOG_ERR, "Missing SDP offer\n");
				error_code = JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT;
				g_snprintf(error_cause, 512, "Missing SDP offer");
				goto error;
			}
			JANUS_VALIDATE_JSON_OBJECT(root, record_parameters,
				error_code, error_cause, TRUE,
				JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT, JANUS_PUSHSTREAM_ERROR_INVALID_ELEMENT);
			if(error_code != 0)
				goto error;
			char error_str[512];
			janus_sdp *offer = janus_sdp_parse(msg_sdp, error_str, sizeof(error_str)), *answer = NULL;
			if(offer == NULL) {
				json_decref(event);
				JANUS_LOG(LOG_ERR, "Error parsing offer: %s\n", error_str);
				error_code = JANUS_PUSHSTREAM_ERROR_INVALID_SDP;
				g_snprintf(error_cause, 512, "Error parsing offer: %s", error_str);
				goto error;
			}
			json_t *name = json_object_get(root, "name");
			const char *name_text = json_string_value(name);
			
			json_t *update = json_object_get(root, "update");
			gboolean do_update = update ? json_is_true(update) : FALSE;
			if(do_update && !sdp_update) {
				JANUS_LOG(LOG_WARN, "Got a 'update' request, but no SDP update? Ignoring...\n");
			}
			json_t *rtmp_pusblisher = json_object_get(root, "rtmp");
			if (NULL == rtmp_pusblisher){
				goto error;
			}
		
			const char *rtmp_pusblisher_text = json_string_value(rtmp_pusblisher);
			if (NULL == strstr(rtmp_pusblisher_text, "rtmp://") || strlen(rtmp_pusblisher_text) <= strlen("rtmp://push-meet.yflive.net/*")) {
				goto error;
			}
		
			
			/* Check if this is a new recorder, or if an update is taking place (i.e., ICE restart) */
			guint64 id = 0;
			janus_pushstream_recording *rec = NULL;
			gboolean audio = FALSE, video = FALSE;
			if(sdp_update) {
				/* Renegotiation: make sure the user provided an offer, and send answer */
				JANUS_LOG(LOG_VERB, "Request to update existing recorder\n");
				if(!session->recorder || !session->recording) {
					JANUS_LOG(LOG_ERR, "Not a recording session, can't update\n");
					error_code = JANUS_PUSHSTREAM_ERROR_INVALID_STATE;
					g_snprintf(error_cause, 512, "Not a recording session, can't update");
					goto error;
				}
				id = session->recording->id;
				rec = session->recording;
				session->sdp_version++;		/* This needs to be increased when it changes */
				audio = (session->arc != NULL);
				video = (session->vrc != NULL);
				sdp_update = do_update;
				goto recdone;
			}
			/* If we're here, we're doing a new recording */
			janus_mutex_lock(&recordings_mutex);
			json_t *rec_id = json_object_get(root, "id");
			if(rec_id) {
				id = json_integer_value(rec_id);
				if(id > 0) {
					/* Let's make sure the ID doesn't exist already */
					if(g_hash_table_lookup(recordings, &id) != NULL) {
						/* It does... */
						janus_mutex_unlock(&recordings_mutex);
						JANUS_LOG(LOG_ERR, "Recording %"SCNu64" already exists!\n", id);
						error_code = JANUS_PUSHSTREAM_ERROR_RECORDING_EXISTS;
						g_snprintf(error_cause, 512, "Recording %"SCNu64" already exists", id);
						goto error;
					}
				}
			}
			if(id == 0) {
				while(id == 0) {
					id = janus_random_uint64();
					if(g_hash_table_lookup(recordings, &id) != NULL) {
						/* Recording ID already taken, try another one */
						id = 0;
					}
				}
			}
			JANUS_LOG(LOG_VERB, "Starting new recording with ID %"SCNu64"\n", id);
			rec = g_malloc0(sizeof(janus_pushstream_recording));
			rec->id = id;
			rec->name = g_strdup(name_text);
			rec->publisher = g_strdup(rtmp_pusblisher_text + strlen("rtmp://"));
			rec->viewers = NULL;
			rec->offer = NULL;
			rec->acodec = JANUS_AUDIOCODEC_NONE;
			rec->vcodec = JANUS_VIDEOCODEC_NONE;
			g_atomic_int_set(&rec->destroyed, 0);
			g_atomic_int_set(&rec->completed, 0);
			janus_refcount_init(&rec->ref, janus_pushstream_recording_free);
			janus_refcount_increase(&rec->ref);	/* This is for the user writing the recording */
			janus_mutex_init(&rec->mutex);
			/* Check which codec we should record for audio and/or video */
			const char *acodec = NULL, *vcodec = NULL;
			janus_sdp_find_preferred_codecs(offer, &acodec, &vcodec);
			
			rec->acodec = janus_audiocodec_from_name("opus");
			rec->vcodec = janus_videocodec_from_name("H264");
			rec->audio_pt = janus_sdp_get_codec_pt(offer, "opus");
			rec->video_pt = janus_sdp_get_codec_pt(offer, "H264");
			//ËµÃ÷ ÕâÁ½¸öÖµ¿ÉÒÔÍ¨¹ý janus_sdp_get_codec_rtpmapº¯Êý²éÑ¯ µ«ÊÇÀïÃæÒ²ÊÇÐ´ËÀµÄ
			rec->audio_channel = 2;
			rec->audio_sample = 48000;

			/* We found preferred codecs: let's just make sure the direction is what we need */
			janus_sdp_mline *m = janus_sdp_mline_find(offer, JANUS_SDP_AUDIO);
			if(m != NULL && m->direction == JANUS_SDP_RECVONLY)
				rec->acodec = JANUS_AUDIOCODEC_NONE;
			audio = (rec->acodec != JANUS_AUDIOCODEC_NONE);
			if(audio) {
				JANUS_LOG(LOG_VERB, "Audio codec: %s\n", janus_audiocodec_name(rec->acodec));
			}
			m = janus_sdp_mline_find(offer, JANUS_SDP_VIDEO);
			if(m != NULL && m->direction == JANUS_SDP_RECVONLY)
				rec->vcodec = JANUS_VIDEOCODEC_NONE;
			video = (rec->vcodec != JANUS_VIDEOCODEC_NONE);
			if(video) {
				JANUS_LOG(LOG_VERB, "Video codec: %s\n", janus_videocodec_name(JANUS_VIDEOCODEC_H264));
			}

			session->rtmp_client_ctx = rtmp_client_init("192.168.1.244", rec->publisher, rec->name, 1935, 2000);
			JANUS_LOG(LOG_INFO, "stream publish to rtmp://192.168.1.244/%s%s\n", rec->publisher, rec->name);
			if (session->rtmp_client_ctx ==NULL)
			{
				error_code = JANUS_PUSHSTREAM_ERROR_CREATE_RTMP_CLIETN_FAILED;
				g_snprintf(error_cause, 512, "create rtmp client failed.");
				goto error;
			}
			session->flv_muxer_ctx = flv_muxer_video_audio_init(session, flv_muxer_callback);
			if (session->flv_muxer_ctx == NULL)
			{
				error_code = JANUS_PUSHSTREAM_ERROR_CREATE_FLV_MUXER_FAILED;
				g_snprintf(error_cause, 512, "create flv muxer failed.");
				goto error;
			}
			/* Create a date string */
			time_t t = time(NULL);
			struct tm *tmv = localtime(&t);
			char outstr[200];
			strftime(outstr, sizeof(outstr), "%Y-%m-%d %H:%M:%S", tmv);
			rec->date = g_strdup(outstr);
			if(audio) {
				session->aac_encode_ctx = aac_encoder_init(rec->audio_channel, rec->audio_sample, 1, 2, 1, aac_encode_callback, session);
				if (session->aac_encode_ctx == NULL)
				{
					error_code = JANUS_PUSHSTREAM_ERROR_CREATE_AAC_ENCODER_FAILED;
					g_snprintf(error_cause, 512, "create aac encoder failed.");
					goto error;
				}
				session->opus_to_pcm_ctx = opus_to_pcm_init(rec->audio_sample, rec->audio_channel, opus_to_pcm_callback, session);
				if (session->opus_to_pcm_ctx == NULL)
				{
					error_code = JANUS_PUSHSTREAM_ERROR_CREATE_OPUS_DECODER_FAILED;
					g_snprintf(error_cause, 512, "create aac encoder failed.");
					goto error;
				}
				session->audio_ctx = rtp_opus_decode_init(rec->audio_pt, "opus", rtp_audio_packet_decode_cb, session, rec->audio_sample, rec->audio_channel);
				if (session->audio_ctx == NULL)
				{
					error_code = JANUS_PUSHSTREAM_ERROR_CREATE_RTP_AUDIO_DECODER_FAILED;
					g_snprintf(error_cause, 512, "create rtp audio decoder failed.");
					goto error;
				}
			}
			if(video) {

				session->video_ctx = rtp_video_decode_init(rec->video_pt, "H264", rtp_video_packet_decode_cb, session);
				if (session->video_ctx == NULL)
				{
					error_code = JANUS_PUSHSTREAM_ERROR_CREATE_RTP_AUDIO_DECODER_FAILED;
					g_snprintf(error_cause, 512, "create rtp video decoder failed.");
					goto error;
				}
			}
			session->recorder = TRUE;
			session->recording = rec;
			session->sdp_version = 1;	/* This needs to be increased when it changes */
			session->sdp_sessid = janus_get_real_time();
			g_hash_table_insert(recordings, janus_uint64_dup(rec->id), rec);
			janus_mutex_unlock(&recordings_mutex);
			/* We need to prepare an answer */
recdone:
			answer = janus_sdp_generate_answer(offer,
				JANUS_SDP_OA_AUDIO, audio,
				JANUS_SDP_OA_AUDIO_CODEC, janus_audiocodec_name(rec->acodec),
				JANUS_SDP_OA_AUDIO_DIRECTION, JANUS_SDP_RECVONLY,
				JANUS_SDP_OA_VIDEO, video,
				JANUS_SDP_OA_VIDEO_CODEC, janus_videocodec_name(rec->vcodec),
				JANUS_SDP_OA_VIDEO_DIRECTION, JANUS_SDP_RECVONLY,
				JANUS_SDP_OA_DATA, FALSE,
				JANUS_SDP_OA_DONE);
			g_free(answer->s_name);
			char s_name[100];
			g_snprintf(s_name, sizeof(s_name), "Recording %"SCNu64, rec->id);
			answer->s_name = g_strdup(s_name);
			/* Let's overwrite a couple o= fields, in case this is a renegotiation */
			answer->o_sessid = session->sdp_sessid;
			answer->o_version = session->sdp_version;
			/* Generate the SDP string */
			sdp = janus_sdp_write(answer);
			janus_sdp_destroy(offer);
			janus_sdp_destroy(answer);
			JANUS_LOG(LOG_VERB, "Going to answer this SDP:\n%s\n", sdp);
			/* If the user negotiated simulcasting, prepare it accordingly */
			json_t *msg_simulcast = json_object_get(msg->jsep, "simulcast");
			if(msg_simulcast) {
				JANUS_LOG(LOG_VERB, "Recording client negotiated simulcasting\n");
				session->ssrc[0] = json_integer_value(json_object_get(msg_simulcast, "ssrc-0"));
				session->ssrc[1] = json_integer_value(json_object_get(msg_simulcast, "ssrc-1"));
				session->ssrc[2] = json_integer_value(json_object_get(msg_simulcast, "ssrc-2"));
				session->sim_context.substream_target = 2;	/* Let's aim for the highest quality */
				session->sim_context.templayer_target = 2;	/* Let's aim for all temporal layers */
				if(rec->vcodec != JANUS_VIDEOCODEC_VP8 && rec->vcodec != JANUS_VIDEOCODEC_H264) {
					/* VP8 r H.264 were not negotiated, if simulcasting was enabled then disable it here */
					session->ssrc[0] = 0;
					session->ssrc[1] = 0;
					session->ssrc[2] = 0;
				}
			}
			/* Done! */
			result = json_object();
			json_object_set_new(result, "status", json_string("recording"));
			json_object_set_new(result, "id", json_integer(id));
			/* Also notify event handlers */
			if(!sdp_update && notify_events && gateway->events_is_enabled()) {
				json_t *info = json_object();
				json_object_set_new(info, "event", json_string("recording"));
				json_object_set_new(info, "id", json_integer(id));
				json_object_set_new(info, "audio", session->arc ? json_true() : json_false());
				json_object_set_new(info, "video", session->vrc ? json_true() : json_false());
				gateway->notify_event(&janus_pushstream_plugin, session->handle, info);
			}
		} else if(!strcasecmp(request_text, "start")) {
			if(!session->aframes && !session->vframes) {
				JANUS_LOG(LOG_ERR, "Not a playout session, can't start\n");
				error_code = JANUS_PUSHSTREAM_ERROR_INVALID_STATE;
				g_snprintf(error_cause, 512, "Not a playout session, can't start");
				goto error;
			}
			/* Just a final message we make use of, e.g., to receive an ANSWER to our OFFER for a playout */
			if(!msg_sdp) {
				JANUS_LOG(LOG_ERR, "Missing SDP answer\n");
				error_code = JANUS_PUSHSTREAM_ERROR_MISSING_ELEMENT;
				g_snprintf(error_cause, 512, "Missing SDP answer");
				goto error;
			}
			/* Done! */
			result = json_object();
			json_object_set_new(result, "status", json_string("playing"));
			/* Also notify event handlers */
			if(notify_events && gateway->events_is_enabled()) {
				json_t *info = json_object();
				json_object_set_new(info, "event", json_string("playing"));
				json_object_set_new(info, "id", json_integer(session->recording->id));
				gateway->notify_event(&janus_pushstream_plugin, session->handle, info);
			}
		} else if(!strcasecmp(request_text, "stop")) {
			/* Done! */
			result = json_object();
			json_object_set_new(result, "status", json_string("stopped"));
			if(session->recording) {
				json_object_set_new(result, "id", json_integer(session->recording->id));
				/* Also notify event handlers */
				if(notify_events && gateway->events_is_enabled()) {
					json_t *info = json_object();
					json_object_set_new(info, "event", json_string("stopped"));
					if(session->recording)
						json_object_set_new(info, "id", json_integer(session->recording->id));
					gateway->notify_event(&janus_pushstream_plugin, session->handle, info);
				}
			}
			/* Stop the recording/playout */
			janus_pushstream_hangup_media(session->handle);
		} else {
			JANUS_LOG(LOG_ERR, "Unknown request '%s'\n", request_text);
			error_code = JANUS_PUSHSTREAM_ERROR_INVALID_REQUEST;
			g_snprintf(error_cause, 512, "Unknown request '%s'", request_text);
			goto error;
		}

		/* Prepare JSON event */
		event = json_object();
		json_object_set_new(event, "pushstream", json_string("event"));
		if(result != NULL)
			json_object_set_new(event, "result", result);
		if(!sdp) {
			int ret = gateway->push_event(msg->handle, &janus_pushstream_plugin, msg->transaction, event, NULL);
			JANUS_LOG(LOG_VERB, "  >> Pushing event: %d (%s)\n", ret, janus_get_api_error(ret));
			json_decref(event);
		} else {
			const char *type = session->recorder ? "answer" : "offer";
			json_t *jsep = json_pack("{ssss}", "type", type, "sdp", sdp);
			if(sdp_update)
				json_object_set_new(jsep, "restart", json_true());
			/* How long will the gateway take to push the event? */
			g_atomic_int_set(&session->hangingup, 0);
			gint64 start = janus_get_monotonic_time();
			int res = gateway->push_event(msg->handle, &janus_pushstream_plugin, msg->transaction, event, jsep);
			JANUS_LOG(LOG_VERB, "  >> Pushing event: %d (took %"SCNu64" us)\n",
				res, janus_get_monotonic_time()-start);
			g_free(sdp);
			json_decref(event);
			json_decref(jsep);
		}
		janus_pushstream_message_free(msg);
		continue;

error:
		{
			/* Prepare JSON error event */
			json_t *event = json_object();
			json_object_set_new(event, "pushstream", json_string("event"));
			json_object_set_new(event, "error_code", json_integer(error_code));
			json_object_set_new(event, "error", json_string(error_cause));
			int ret = gateway->push_event(msg->handle, &janus_pushstream_plugin, msg->transaction, event, NULL);
			JANUS_LOG(LOG_VERB, "  >> Pushing event: %d (%s)\n", ret, janus_get_api_error(ret));
			json_decref(event);
			janus_pushstream_message_free(msg);
		}
	}
	JANUS_LOG(LOG_VERB, "LeavingRecord&Play handler thread\n");
	return NULL;
}

static void rtp_video_packet_decode_cb(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	janus_pushstream_session *session = (janus_pushstream_session *)param;
	const uint8_t start_code[] = {0, 0, 0, 1 };

#ifdef TEST_DEBUG
	fwrite(start_code, 1, 4, session->video_ctx->fp);
	fwrite(packet, 1, bytes, session->video_ctx->fp);
#endif // TEST_DEBUG

	uint8_t * ptr = (uint8_t*)packet;
	/*7 sps   
	8 pps  都不发送*/
	if (session->video_ctx->capacity- session->video_ctx->used_len < bytes+4)
	{
		session->video_ctx->pData = (char*)realloc(session->video_ctx->pData, session->video_ctx->capacity + bytes + 4);
	}
	memcpy(session->video_ctx->pData + session->video_ctx->used_len, start_code, 4);
	session->video_ctx->used_len += 4;
	memcpy(session->video_ctx->pData + session->video_ctx->used_len, packet, bytes);
	session->video_ctx->used_len += bytes;

	uint8_t nalutype = 0x1F & ptr[0];
	if (nalutype == 7 || nalutype == 8)
	{
		//不发送  和I帧一起发送
	}
	else
	{
		if (5 == nalutype) {
			JANUS_LOG(LOG_VERB, "%s %s  got a keyframe \n", session->recording->publisher, session->recording->name);
		}
		flv_muxer_video_audio_input(session->flv_muxer_ctx, 27, timestamp, timestamp, session->video_ctx->pData, session->video_ctx->used_len);
		session->video_ctx->used_len = 0;
	}

}

//param对应的是 struct rtp_opus_context_t* 因为在初始化时，session的 audio_ctx还没有被赋值，如果返回session，那么session->audio_ctx为NULL
static void rtp_audio_packet_decode_cb(void* param, const void *packet, int bytes, uint32_t timestamp, int flags)
{
	struct rtp_opus_context_t* ctx = (struct rtp_opus_context_t*)param;
	janus_pushstream_session *session = (janus_pushstream_session *)ctx->cbdata;
	opus_to_pcm_decode(session->opus_to_pcm_ctx, packet, bytes, timestamp, flags);

#ifdef TEST_DEBUG
	fwrite(packet, bytes, 1, ctx->fp);
#endif // TEST_DEBUG

}

static void opus_to_pcm_callback(void* param, unsigned char* pdata, int len, uint32_t timestamp)
{
	janus_pushstream_session *session = (janus_pushstream_session *)param;
	aac_encode_input(session->aac_encode_ctx, pdata, len, timestamp);

#ifdef TEST_DEBUG
	fwrite(pdata, len, 1, session->opus_to_pcm_ctx->fd);
#endif 

}

static void aac_encode_callback(void* parame, unsigned char* pdata, int len, uint32_t timestamp)
{
	janus_pushstream_session *session = (janus_pushstream_session *)parame;

#ifdef TEST_DEBUG
	fwrite(pdata, len, 1, session->aac_encode_ctx->fd);
#endif 

	flv_muxer_video_audio_input(session->flv_muxer_ctx,15,timestamp,timestamp,pdata,len);
}

static void flv_muxer_callback(void* parame, int type, const void* pdata, size_t len, uint32_t timestamp)
{
	janus_pushstream_session *session = (janus_pushstream_session *)parame;
	uint8_t tag[11];
	//rtmp不需要发送这些数据

#ifdef TEST_DEBUG
	flv_write_tag(tag, (uint8_t)type, (uint32_t)len, timestamp);
	fwrite(tag, 11, 1, session->flv_muxer_ctx->fd); // FLV Tag Header
	fwrite(pdata, len, 1, session->flv_muxer_ctx->fd);
#endif 	

	rtmp_client_input_flv(session->rtmp_client_ctx, pdata, len, type, timestamp);

#ifdef TEST_DEBUG
	be_write_uint32(tag, (uint32_t)len + 11);
	fwrite(tag, 4, 1, session->flv_muxer_ctx->fd);
#endif 	
}

