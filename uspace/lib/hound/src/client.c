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

/** @addtogroup libhound
 * @addtogroup audio
 * @{
 */
/** @file
 * Common USB functions.
 */
#include <adt/list.h>
#include <errno.h>
#include <loc.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>
#include <libarch/types.h>

#include "protocol.h"
#include "client.h"

/***
 * CLIENT SIDE
 ***/

typedef struct hound_stream {
	link_t link;
	pcm_format_t format;
	async_exch_t *exch;
	hound_context_t *context;
} hound_stream_t;

typedef struct hound_context {
	hound_sess_t *session;
	const char *name;
	bool record;
	list_t stream_list;
	struct {
		hound_stream_t *stream;
		pcm_format_t format;
		size_t bsize;
	} main;
	hound_context_id_t id;
} hound_context_t;


static hound_context_t *hound_context_create(const char *name, bool record,
    pcm_format_t format, size_t bsize)
{
	hound_context_t *new_context = malloc(sizeof(hound_context_t));
	if (new_context) {
		char *cont_name;
		int ret = asprintf(&cont_name, "%llu:%s", task_get_id(), name);
		if (ret < 0) {
			free(new_context);
			return NULL;
		}
		list_initialize(&new_context->stream_list);
		new_context->name = cont_name;
		new_context->record = record;
		new_context->session = hound_service_connect(HOUND_SERVICE);
		new_context->main.stream = NULL;
		new_context->main.format = format;
		new_context->main.bsize = bsize;
		if (!new_context->session) {
			free(new_context->name);
			free(new_context);
			return NULL;
		}
		new_context->id = hound_service_register_context(
		    new_context->session, new_context->name, record);
		if (hound_context_id_err(new_context->id) != EOK) {
			hound_service_disconnect(new_context->session);
			free(new_context->name);
			free(new_context);
			return NULL;
		}
	}
	return new_context;
}

hound_context_t * hound_context_create_playback(const char *name,
    pcm_format_t format, size_t bsize)
{
	return hound_context_create(name, false, format, bsize);
}

hound_context_t * hound_context_create_capture(const char *name,
    pcm_format_t format, size_t bsize)
{
	return hound_context_create(name, true, format, bsize);
}

void hound_context_destroy(hound_context_t *hound)
{
	assert(hound);
	hound_service_unregister_context(hound->session, hound->id);
	hound_service_disconnect(hound->session);
	free(hound->name);
	free(hound);
}

int hound_context_enable(hound_context_t *hound)
{
	assert(hound);
	return ENOTSUP;
}
int hound_context_disable(hound_context_t *hound)
{
	assert(hound);
	return ENOTSUP;
}

int hound_context_get_available_targets(hound_context_t *hound,
    const char ***names, size_t *count)
{
	assert(hound);
	assert(names);
	assert(count);
	return hound_service_get_list_all(hound->session, names, count,
	    hound->record ? HOUND_SOURCE_DEVS : HOUND_SINK_DEVS);
}

int hound_context_get_connected_targets(hound_context_t *hound,
    const char ***names, size_t *count)
{
	assert(hound);
	assert(names);
	assert(count);
	return hound_service_get_list(hound->session, names, count,
	    HOUND_CONNECTED | (hound->record ?
	        HOUND_SOURCE_DEVS : HOUND_SINK_DEVS), hound->name);
}

int hound_context_connect_target(hound_context_t *hound, const char* target)
{
	assert(hound);
	assert(target);

	const char **tgt = NULL;
	size_t count = 1;
	int ret = EOK;
	if (str_cmp(target, HOUND_DEFAULT_TARGET) == 0) {
		ret = hound_context_get_available_targets(hound, &tgt, &count);
		if (ret != EOK)
			return ret;
		target = tgt[0];
	}
	//TODO handle all-targets

	if (hound->record) {
		ret = hound_service_connect_source_sink(
		    hound->session, target, hound->name);
	} else {
		ret = hound_service_connect_source_sink(
		    hound->session, hound->name, target);
	}
	if (tgt)
		free(tgt[0]);
	free(tgt);
	return ret;
}

int hound_context_disconnect_target(hound_context_t *hound, const char* target)
{
	assert(hound);
	assert(target);
	//TODO handle all-targets
	if (hound->record) {
		return hound_service_disconnect_source_sink(
		    hound->session, target, hound->name);
	} else {
		return hound_service_disconnect_source_sink(
		    hound->session, hound->name, target);
	}
}

hound_stream_t *hound_stream_create(hound_context_t *hound, unsigned flags,
    pcm_format_t format, size_t bsize)
{
	assert(hound);
	async_exch_t *stream_exch = async_exchange_begin(hound->session);
	if (!stream_exch)
		return NULL;
	hound_stream_t *new_stream = malloc(sizeof(hound_stream_t));
	if (new_stream) {
		link_initialize(&new_stream->link);
		new_stream->exch = stream_exch;
		new_stream->format = format;
		new_stream->context = hound;
		const int ret = hound_service_stream_enter(new_stream->exch,
		    hound->id, flags, format, bsize);
		if (ret != EOK) {
			async_exchange_end(new_stream->exch);
			free(new_stream);
			return NULL;
		}
		list_append(&new_stream->link, &hound->stream_list);
	}
	return new_stream;
}

void hound_stream_destroy(hound_stream_t *stream)
{
	if (stream) {
		// TODO drain?
		hound_service_stream_exit(stream->exch);
		async_exchange_end(stream->exch);
		list_remove(&stream->link);
		free(stream);
	}
}

int hound_stream_write(hound_stream_t *stream, const void *data, size_t size)
{
	assert(stream);
	if (!data || size == 0)
		return EBADMEM;
	return hound_service_stream_write(stream->exch, data, size);
}

int hound_stream_read(hound_stream_t *stream, void *data, size_t size)
{
	assert(stream);
	if (!data || size == 0)
		return EBADMEM;
	return hound_service_stream_read(stream->exch, data, size);
}

static hound_stream_t * hound_get_main_stream(hound_context_t *hound)
{
	assert(hound);
	if (!hound->main.stream)
		hound->main.stream = hound_stream_create(hound, 0,
		    hound->main.format, hound->main.bsize);
	return hound->main.stream;
}

int hound_write_main_stream(hound_context_t *hound,
    const void *data, size_t size)
{
	assert(hound);
	hound_stream_t *mstream = hound_get_main_stream(hound);
	if (!mstream)
		return ENOMEM;
	return hound_stream_write(mstream, data, size);
}

int hound_read_main_stream(hound_context_t *hound, void *data, size_t size)
{
	assert(hound);
	hound_stream_t *mstream = hound_get_main_stream(hound);
	if (!mstream)
		return ENOMEM;
	return hound_stream_read(mstream, data, size);
}

int hound_write_replace_main_stream(hound_context_t *hound,
    const void *data, size_t size)
{
	assert(hound);
	if (!data || size == 0)
		return EBADMEM;
	// TODO implement
	return ENOTSUP;
}

int hound_context_set_main_stream_format(hound_context_t *hound,
    unsigned channels, unsigned rate, pcm_sample_format_t format)
{
	assert(hound);
	// TODO implement
	return ENOTSUP;
}

int hound_write_immediate(hound_context_t *hound, pcm_format_t format,
    const void *data, size_t size)
{
	assert(hound);
	hound_stream_t *tmpstream = hound_stream_create(hound, 0, format, size);
	if (!tmpstream)
		return ENOMEM;
	const int ret = hound_stream_write(tmpstream, data, size);
	if (ret == EOK) {
		//TODO drain?
		hound_service_stream_drain(tmpstream->exch);
	}
	hound_stream_destroy(tmpstream);
	return ret;
}


/**
 * @}
 */
