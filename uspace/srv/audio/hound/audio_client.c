/*
 * Copyright (c) 2012 Jan Vesely
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>

#include "audio_client.h"
#include "log.h"

static void init_common(audio_client_t *client, const char *name,
    const audio_format_t *f, async_sess_t *sess)
{
	link_initialize(&client->link);
	client->name = str_dup(name);
	client->format = *f;
	client->sess = sess;
	client->exch = NULL;
	client->is_playback = false;
	client->is_recording = false;
}

static int client_sink_connection_change(audio_sink_t *sink, bool new);
static int client_source_connection_change(audio_source_t *source);
static int client_source_update_data(audio_source_t *source, size_t size);


audio_client_t *audio_client_get_playback(
    const char *name, const audio_format_t *f, async_sess_t *sess)
{
	audio_client_t *client = malloc(sizeof(audio_client_t));
	if (!client)
		return NULL;
	init_common(client, name, f, sess);
	audio_source_init(&client->source, name, client,
	    client_source_connection_change, client_source_update_data, f);
	client->is_playback = true;
	return client;
}

audio_client_t *audio_client_get_recording(
    const char *name, const audio_format_t *f, async_sess_t *sess)
{
	audio_client_t *client = malloc(sizeof(audio_client_t));
	if (!client)
		return NULL;
	init_common(client, name, f, sess);
	audio_sink_init(&client->sink, name, client,
	    client_sink_connection_change, f);
	client->is_recording = true;
	return client;
}

void audio_client_destroy(audio_client_t *client)
{
	if (!client)
		return;
	if (client->is_recording) {
		/* There is a fibril running */
		client->is_recording = false;
		return;
	}
	async_exchange_end(client->exch);
	async_hangup(client->sess);
	free(client->name);
	if (client->is_playback) {
		audio_source_fini(&client->source);
	} else { /* was recording */
		audio_sink_fini(&client->sink);
	}
	free(client);
}

static int client_sink_connection_change(audio_sink_t *sink, bool new)
{
	//TODO create fibril
	return ENOTSUP;
}

static int client_source_connection_change(audio_source_t *source)
{
	assert(source);
	audio_client_t *client = source->private_data;
	if (source->connected_sink) {
		client->exch = async_exchange_begin(client->sess);
		return client->exch ? EOK : ENOMEM;
	}
	async_exchange_end(client->exch);
	client->exch = NULL;
	return EOK;
}

static int client_source_update_data(audio_source_t *source, size_t size)
{
	assert(source);
	audio_client_t *client = source->private_data;
	void *buffer = malloc(size);
	if (!buffer)
		return ENOMEM;
	assert(client->exch);
	const int ret = async_data_read_start(client->exch, buffer, size);
	if (ret != EOK) {
		log_debug("Failed to read data from client");
		free(buffer);
		return ret;
	}
	void *old_buffer = source->available_data.base;
	source->available_data.position = buffer;
	source->available_data.base = buffer;
	source->available_data.size = size;
	free(old_buffer);

	return EOK;
}
