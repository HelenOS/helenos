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

#include "client.h"
#include "protocol.h"

/***
 * CLIENT SIDE
 ***/

typedef struct hound_stream {
	link_t link;
	async_exch_t *exch;
} hound_stream_t;

typedef struct hound_context {
	hound_sess_t *session;
	const char *name;
	bool record;
	list_t stream_list;
	hound_stream_t *main_stream;
	struct {
		pcm_sample_format_t sample;
		unsigned channels;
		unsigned rate;
		size_t bsize;
	} main_format;
	unsigned id;
} hound_context_t;


static hound_context_t *hound_context_create(const char *name, bool record,
    unsigned channels, unsigned rate, pcm_sample_format_t format, size_t bsize)
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
		new_context->main_stream = NULL;
		new_context->main_format.sample = format;
		new_context->main_format.rate = rate;
		new_context->main_format.channels = channels;
		new_context->main_format.bsize = bsize;
		if (!new_context->session) {
			free(new_context->name);
			free(new_context);
			return NULL;
		}
		async_exch_t *exch = async_exchange_begin(new_context->session);
		//TODO: register context
		async_exchange_end(exch);
	}
	return new_context;
}

hound_context_t * hound_context_create_playback(const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format, size_t bsize)
{
	return hound_context_create(name, false, channels, rate, format, bsize);
}

hound_context_t * hound_context_create_capture(const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format, size_t bsize)
{
	return hound_context_create(name, true, channels, rate, format, bsize);
}

void hound_context_destroy(hound_context_t *hound)
{
	assert(hound);
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

int hound_get_output_targets(const char **names, size_t *count)
{
	assert(names);
	assert(count);
	return ENOTSUP;
}

int hound_get_input_targets(const char **names, size_t *count)
{
	assert(names);
	assert(count);
	return ENOTSUP;
}

int hound_context_connect_target(hound_context_t *hound, const char* target)
{
	assert(hound);
	return ENOTSUP;
}

int hound_context_disconnect_target(hound_context_t *hound, const char* target)
{
	assert(hound);
	return ENOTSUP;
}

int hound_context_set_main_stream_format(hound_context_t *hound,
    unsigned channels, unsigned rate, pcm_sample_format_t format);
int hound_write_main_stream(hound_context_t *hound,
    const void *data, size_t size);
int hound_read_main_stream(hound_context_t *hound, void *data, size_t size);
int hound_write_replace_main_stream(hound_context_t *hound,
    const void *data, size_t size);
int hound_write_immediate_stream(hound_context_t *hound,
    const void *data, size_t size);

/**
 * @}
 */
