/*
 * ws protocol handler plugin for "POST demo"
 *
 * Copyright (C) 2010-2016 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * These test plugins are intended to be adapted for use in your code, which
 * may be proprietary.  So unlike the library itself, they are licensed
 * Public Domain.
 */

#define LWS_DLL
#define LWS_INTERNAL
#include "../lib/libwebsockets.h"

#include <string.h>

struct per_session_data__post_demo {
	struct lws_urldecode_stateful_param_array *spa;
	char result[500 + LWS_PRE];
	int result_len;
};

static const char * const param_names[] = {
	"Text",
};

enum enum_param_names {
	EPN_TEXT,
};

static int
callback_post_demo(struct lws *wsi, enum lws_callback_reasons reason,
		   void *user, void *in, size_t len)
{
	struct per_session_data__post_demo *pss =
			(struct per_session_data__post_demo *)user;
	unsigned char buffer[LWS_PRE + 512];
	unsigned char *p, *start, *end;
	int n;

	switch (reason) {

	case LWS_CALLBACK_HTTP_BODY:
		/* create the POST argument parser if not already existing */
		if (!pss->spa) {
			pss->spa = lws_urldecode_spa_create(param_names,
					ARRAY_SIZE(param_names), 1024,
					NULL, NULL);
			if (!pss->spa)
				return -1;
		}

		/* let it parse the POST data */
		if (lws_urldecode_spa_process(pss->spa, in, len))
			return -1;
		break;

	case LWS_CALLBACK_HTTP_WRITEABLE:
		lwsl_debug("LWS_CALLBACK_HTTP_WRITEABLE: sending %d\n",
			   pss->result_len);
		n = lws_write(wsi, (unsigned char *)pss->result + LWS_PRE,
			      pss->result_len, LWS_WRITE_HTTP);
		if (n < 0)
			return 1;
		goto try_to_reuse;

	case LWS_CALLBACK_HTTP_BODY_COMPLETION:
		lwsl_debug("LWS_CALLBACK_HTTP_BODY_COMPLETION\n");
		/*
		 * the whole of the sent body arrived,
		 * respond to the client with a redirect to show the
		 * results
		 */

		/* call to inform no more payload data coming */
		lws_urldecode_spa_finalize(pss->spa);

		pss->result_len = sprintf((char *)pss->result + LWS_PRE,
			    "<html><body><h1>Form results</h1>"
			    "<b>Text (after urldecoding)</b>:'%s' (len %d)<br>"
			    "</body></html>",
			    lws_urldecode_spa_get_string(pss->spa, EPN_TEXT),
			    lws_urldecode_spa_get_length(pss->spa, EPN_TEXT));

		/* finished with the POST argument parser */
//		lws_urldecode_spa_destroy(pss->spa);
//		pss->spa = NULL;

		p = buffer + LWS_PRE;
		start = p;
		end = p + sizeof(buffer) - LWS_PRE;

		if (lws_add_http_header_status(wsi, 200, &p, end))
			return 1;

		if (lws_add_http_header_by_token(wsi, WSI_TOKEN_HTTP_CONTENT_TYPE,
				(unsigned char *)"text/html", 9, &p, end))
			return 1;
		if (lws_add_http_header_content_length(wsi, pss->result_len, &p, end))
			return 1;
		if (lws_finalize_http_header(wsi, &p, end))
			return 1;

		n = lws_write(wsi, start, p - start, LWS_WRITE_HTTP_HEADERS);
		if (n < 0)
			return 1;

		/*
		 *  send the payload next time, in case would block after
		 * headers
		 */
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
		if (pss->spa) {
			lws_urldecode_spa_destroy(pss->spa);
			pss->spa = NULL;
		}
		break;

	default:
		break;
	}

	return 0;

try_to_reuse:
	if (lws_http_transaction_completed(wsi))
		return -1;

	return 0;
}

static const struct lws_protocols protocols[] = {
	{
		"protocol-post-demo",
		callback_post_demo,
		sizeof(struct per_session_data__post_demo),
		1024,
	},
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_post_demo(struct lws_context *context,
			struct lws_plugin_capability *c)
{
	if (c->api_magic != LWS_PLUGIN_API_MAGIC) {
		lwsl_err("Plugin API %d, library API %d", LWS_PLUGIN_API_MAGIC,
			 c->api_magic);
		return 1;
	}

	c->protocols = protocols;
	c->count_protocols = ARRAY_SIZE(protocols);
	c->extensions = NULL;
	c->count_extensions = 0;

	return 0;
}

LWS_EXTERN LWS_VISIBLE int
destroy_protocol_post_demo(struct lws_context *context)
{
	return 0;
}
