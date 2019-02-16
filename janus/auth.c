/*! \file    auth.c
 * \author   Lorenzo Miniero <lorenzo@meetecho.com>
 * \copyright GNU General Public License v3
 * \brief    Requests authentication
 * \details  Implementation of simple mechanisms for authenticating
 * requests.
 *
 * If enabled (it's disabled by default), each request must contain a
 * valid token string, * or otherwise the request is rejected with an
 * error.
 *
 * When no \c token_auth_secret is set, Stored-token mode is active.
 * In this mode the Janus admin API can be used to specify valid string
 * tokens. Whether tokens should be shared across users or not is
 * completely up to the controlling application: these tokens are
 * completely opaque to Janus, and treated as strings, which means
 * Janus will only check if the token exists or not when asked.
 *
 * However, if a secret is set, the Signed-token mode is used.
 * In this mode, no direct communication between the controlling
 * application and Janus is necessary. Instead, the application signs
 * tokens that Janus can verify using the secret key.
 *
 * \ingroup core
 * \ref core
 */

#include <string.h>
#include <openssl/hmac.h>

#include "auth.h"
#include "debug.h"
#include "mutex.h"
#include "utils.h"
#include <curl/curl.h>
#include <jansson.h>
#include "janus.h"


/* Hash table to contain the tokens to match */
static GHashTable *tokens = NULL, *allowed_plugins = NULL;
static gboolean auth_enabled = FALSE;
static janus_mutex mutex;
static char *auth_secret = NULL;

static void janus_auth_free_token(char *token) {
	g_free(token);
}

/* Setup */
void janus_auth_init(gboolean enabled, const char *secret) {
	if(enabled) {
		if(secret == NULL) {
			JANUS_LOG(LOG_INFO, "Stored-Token based authentication enabled\n");
			tokens = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)janus_auth_free_token, NULL);
			allowed_plugins = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)janus_auth_free_token, NULL);
			auth_enabled = TRUE;
		} else {
			JANUS_LOG(LOG_INFO, "Signed-Token based authentication enabled\n");
			auth_secret = g_strdup(secret);
			auth_enabled = TRUE;
		}
	} else {
		JANUS_LOG(LOG_WARN, "Token based authentication disabled\n");
	}
	janus_mutex_init(&mutex);
}

gboolean janus_auth_is_enabled(void) {
	return auth_enabled;
}

gboolean janus_auth_is_stored_mode(void) {
	return auth_enabled && tokens != NULL;
}

void janus_auth_deinit(void) {
	janus_mutex_lock(&mutex);
	if(tokens != NULL)
		g_hash_table_destroy(tokens);
	tokens = NULL;
	if(allowed_plugins != NULL)
		g_hash_table_destroy(allowed_plugins);
	allowed_plugins = NULL;
	g_free(auth_secret);
	auth_secret = NULL;
	janus_mutex_unlock(&mutex);
}

gboolean janus_auth_check_signature(const char *token, const char *realm) {
	if (!auth_enabled || auth_secret == NULL)
		return FALSE;
	gchar **parts = g_strsplit(token, ":", 2);
	gchar **data = NULL;
	/* Token should have exactly one data and one hash part */
	if(!parts[0] || !parts[1] || parts[2])
		goto fail;
	data = g_strsplit(parts[0], ",", 3);
	/* Need at least an expiry timestamp and realm */
	if(!data[0] || !data[1])
		goto fail;
	/* Verify timestamp */
	gint64 expiry_time = atoi(data[0]);
	gint64 real_time = janus_get_real_time() / 1000000;
	if(real_time > expiry_time)
		goto fail;
	/* Verify realm */
	if(strcmp(data[1], realm))
		goto fail;
	/* Verify HMAC-SHA1 */
	unsigned char signature[EVP_MAX_MD_SIZE];
	unsigned int len;
	HMAC(EVP_sha1(), auth_secret, strlen(auth_secret), (const unsigned char*)parts[0], strlen(parts[0]), signature, &len);
	gchar *base64 = g_base64_encode(signature, len);
	gboolean result = janus_strcmp_const_time(parts[1], base64);
	g_strfreev(data);
	g_strfreev(parts);
	g_free(base64);
	return result;

fail:
	g_strfreev(data);
	g_strfreev(parts);
	return FALSE;
}

gboolean janus_auth_check_signature_contains(const char *token, const char *realm, const char *desc) {
	if (!auth_enabled || auth_secret == NULL)
		return FALSE;
	gchar **parts = g_strsplit(token, ":", 2);
	gchar **data = NULL;
	/* Token should have exactly one data and one hash part */
	if(!parts[0] || !parts[1] || parts[2])
		goto fail;
	data = g_strsplit(parts[0], ",", 0);
	/* Need at least an expiry timestamp and realm */
	if(!data[0] || !data[1])
		goto fail;
	/* Verify timestamp */
	gint64 expiry_time = atoi(data[0]);
	gint64 real_time = janus_get_real_time() / 1000000;
	if(real_time > expiry_time)
		goto fail;
	/* Verify realm */
	if(strcmp(data[1], realm))
		goto fail;
	/* Find descriptor */
	gboolean result = FALSE;
	int i = 2;
	for(i = 2; data[i]; i++) {
		if (!strcmp(desc, data[i])) {
			result = TRUE;
			break;
		}
	}
	if (!result)
		goto fail;
	/* Verify HMAC-SHA1 */
	unsigned char signature[EVP_MAX_MD_SIZE];
	unsigned int len;
	HMAC(EVP_sha1(), auth_secret, strlen(auth_secret), (const unsigned char*)parts[0], strlen(parts[0]), signature, &len);
	gchar *base64 = g_base64_encode(signature, len);
	result = janus_strcmp_const_time(parts[1], base64);
	g_strfreev(data);
	g_strfreev(parts);
	g_free(base64);
	return result;

fail:
	g_strfreev(data);
	g_strfreev(parts);
	return FALSE;
}

/* Tokens manipulation */
gboolean janus_auth_add_token(const char *token) {
	if(!auth_enabled || tokens == NULL) {
		JANUS_LOG(LOG_ERR, "Can't add token, stored-authentication mechanism is disabled\n");
		return FALSE;
	}
	if(token == NULL)
		return FALSE;
	janus_mutex_lock(&mutex);
	if(g_hash_table_lookup(tokens, token)) {
		JANUS_LOG(LOG_VERB, "Token already validated\n");
		janus_mutex_unlock(&mutex);
		return TRUE;
	}
	char *new_token = g_strdup(token);
	g_hash_table_insert(tokens, new_token, new_token);
	janus_mutex_unlock(&mutex);
	return TRUE;
}

gboolean janus_auth_check_token(const char *token) {
	/* Always TRUE if the mechanism is disabled, of course */
	if(!auth_enabled)
		return TRUE;
	if (tokens == NULL)
		return janus_auth_check_signature(token, "janus");
	janus_mutex_lock(&mutex);
	if(token && g_hash_table_lookup(tokens, token)) {
		janus_mutex_unlock(&mutex);
		return TRUE;
	}
	janus_mutex_unlock(&mutex);
	return FALSE;
}

GList *janus_auth_list_tokens(void) {
	/* Always NULL if the mechanism is disabled, of course */
	if(!auth_enabled || tokens == NULL)
		return NULL;
	janus_mutex_lock(&mutex);
	GList *list = NULL;
	if(g_hash_table_size(tokens) > 0) {
		GHashTableIter iter;
		gpointer value;
		g_hash_table_iter_init(&iter, tokens);
		while (g_hash_table_iter_next(&iter, NULL, &value)) {
			const char *token = value;
			list = g_list_append(list, g_strdup(token));
		}
	}
	janus_mutex_unlock(&mutex);
	return list;
}

gboolean janus_auth_remove_token(const char *token) {
	if(!auth_enabled || tokens == NULL) {
		JANUS_LOG(LOG_ERR, "Can't remove token, stored-authentication mechanism is disabled\n");
		return FALSE;
	}
	janus_mutex_lock(&mutex);
	gboolean ok = token && g_hash_table_remove(tokens, token);
	/* Also clear the allowed plugins mapping */
	GList *list = g_hash_table_lookup(allowed_plugins, token);
	g_hash_table_remove(allowed_plugins, token);
	if(list != NULL)
		g_list_free(list);
	/* Done */
	janus_mutex_unlock(&mutex);
	return ok;
}

/* Plugins access */
gboolean janus_auth_allow_plugin(const char *token, janus_plugin *plugin) {
	if(!auth_enabled || allowed_plugins == NULL) {
		JANUS_LOG(LOG_ERR, "Can't allow access to plugin, authentication mechanism is disabled\n");
		return FALSE;
	}
	if(token == NULL || plugin == NULL)
		return FALSE;
	janus_mutex_lock(&mutex);
	if(!g_hash_table_lookup(tokens, token)) {
		janus_mutex_unlock(&mutex);
		return FALSE;
	}
	GList *list = g_hash_table_lookup(allowed_plugins, token);
	if(list == NULL) {
		/* Add the new permission now */
		list = g_list_append(list, plugin);
		char *new_token = g_strdup(token);
		g_hash_table_insert(allowed_plugins, new_token, list);
		janus_mutex_unlock(&mutex);
		return TRUE;
	}
	/* We already have a list, update it if needed */
	if(g_list_find(list, plugin) != NULL) {
		JANUS_LOG(LOG_VERB, "Plugin access already allowed for token\n");
		janus_mutex_unlock(&mutex);
		return TRUE;
	}
	list = g_list_append(list, plugin);
	char *new_token = g_strdup(token);
	g_hash_table_insert(allowed_plugins, new_token, list);
	janus_mutex_unlock(&mutex);
	return TRUE;
}

gboolean janus_auth_check_plugin(const char *token, janus_plugin *plugin) {
	/* Always TRUE if the mechanism is disabled, of course */
	if(!auth_enabled)
		return TRUE;
	if (allowed_plugins == NULL)
		return janus_auth_check_signature_contains(token, "janus", plugin->get_package());
	janus_mutex_lock(&mutex);
	if(!g_hash_table_lookup(tokens, token)) {
		janus_mutex_unlock(&mutex);
		return FALSE;
	}
	GList *list = g_hash_table_lookup(allowed_plugins, token);
	if(g_list_find(list, plugin) == NULL) {
		janus_mutex_unlock(&mutex);
		return FALSE;
	}
	janus_mutex_unlock(&mutex);
	return TRUE;
}

GList *janus_auth_list_plugins(const char *token) {
	/* Always NULL if the mechanism is disabled, of course */
	if(!auth_enabled || allowed_plugins == NULL)
		return NULL;
	janus_mutex_lock(&mutex);
	if(!g_hash_table_lookup(tokens, token)) {
		janus_mutex_unlock(&mutex);
		return FALSE;
	}
	GList *list = NULL;
	GList *plugins_list = g_hash_table_lookup(allowed_plugins, token);
	if(plugins_list != NULL)
		list = g_list_copy(plugins_list);
	janus_mutex_unlock(&mutex);
	return list;
}

gboolean janus_auth_disallow_plugin(const char *token, janus_plugin *plugin) {
	if(!auth_enabled || allowed_plugins == NULL) {
		JANUS_LOG(LOG_ERR, "Can't disallow access to plugin, authentication mechanism is disabled\n");
		return FALSE;
	}
	janus_mutex_lock(&mutex);
	if(!g_hash_table_lookup(tokens, token)) {
		janus_mutex_unlock(&mutex);
		return FALSE;
	}
	GList *list = g_hash_table_lookup(allowed_plugins, token);
	if(list != NULL) {
		/* Update the list */
		list = g_list_remove_all(list, plugin);
		char *new_token = g_strdup(token);
		g_hash_table_insert(allowed_plugins, new_token, list);
	}
	janus_mutex_unlock(&mutex);
	return TRUE;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	memcpy(stream, ptr, nmemb > 1024 ? 1024 : nmemb);
	return size * nmemb;
}

gboolean janus_auth_personal_auth(const char *auth_url, const char *user, const char *sign, janus_plugin *plugin){
	JANUS_LOG(LOG_ERR, "[janus_auth_personal_auth] %s", auth_url);
	if (user == NULL || sign == NULL || auth_url == NULL )
		return FALSE;

	char url[256] = { 0 };
	char buf[1024] = { 0 };


	g_snprintf( url, 256, "%s?company_id=%s&auth_id=%s", auth_url, user, sign );
	void* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		JANUS_LOG(LOG_ERR, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	}
	curl_easy_cleanup(curl);

	json_error_t err;
	json_t* root = json_loads(buf, 1024, &err);
	if (NULL != root && json_is_object(root)){
		json_t* errCode = json_object_get(root, "errCode");
		if (NULL != errCode)
		{
			int errCode_value = json_integer_value(errCode);
			if ( errCode_value !=0 )
			{
				return FALSE;
			}
		}
		return TRUE;
	} else {
		return TRUE;
	}
}
