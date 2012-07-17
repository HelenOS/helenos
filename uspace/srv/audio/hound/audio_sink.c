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
#include <str_error.h>

#include "audio_sink.h"
#include "log.h"


int audio_sink_init(audio_sink_t *sink, const char *name,
    void *private_data, int (*connection_change)(audio_sink_t *, bool),
    int (*check_format)(audio_sink_t *sink), const pcm_format_t *f)
{
	assert(sink);
	if (!name) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	link_initialize(&sink->link);
	list_initialize(&sink->sources);
	sink->name = str_dup(name);
	sink->private_data = private_data;
	sink->format = *f;
	sink->connection_change = connection_change;
	sink->check_format = check_format;
	log_verbose("Initialized sink (%p) '%s'", sink, sink->name);
	return EOK;
}

void audio_sink_fini(audio_sink_t *sink)
{
	assert(sink);
	assert(!sink->private_data);
	free(sink->name);
	sink->name = NULL;
}

int audio_sink_add_source(audio_sink_t *sink, audio_source_t *source)
{
	assert(sink);
	assert(source);
	assert_link_not_used(&source->link);
	list_append(&source->link, &sink->sources);

	const pcm_format_t old_format = sink->format;

	/* The first source for me */
	if (list_count(&sink->sources) == 1) {
		/* Set audio format according to the first source */
		if (pcm_format_is_any(&sink->format)) {
			int ret = audio_sink_set_format(sink, &source->format);
			if (ret != EOK)
				return ret;
		}
	}

	audio_source_connected(source, sink);

	if (sink->connection_change) {
		log_verbose("Calling connection change");
		const int ret = sink->connection_change(sink, true);
		if (ret != EOK) {
			log_debug("Connection hook failed.");
			audio_source_connected(source, NULL);
			list_remove(&source->link);
			sink->format = old_format;
			return ret;
		}
	}
	log_verbose("Connected source '%s' to sink '%s'",
	    source->name, sink->name);

	return EOK;
}

int audio_sink_set_format(audio_sink_t *sink, const pcm_format_t *format)
{
	assert(sink);
	assert(format);
	if (!pcm_format_is_any(&sink->format)) {
		log_debug("Sink %s already has a format", sink->name);
		return EEXISTS;
	}
	const pcm_format_t old_format;

	if (pcm_format_is_any(format)) {
		log_verbose("Setting DEFAULT format for sink %s", sink->name);
		sink->format = AUDIO_FORMAT_DEFAULT;
	} else {
		sink->format = *format;
	}
	if (sink->check_format) {
		const int ret = sink->check_format(sink);
		if (ret != EOK && ret != ELIMIT) {
			log_debug("Format check failed on sink %s", sink->name);
			sink->format = old_format;
			return ret;
		}
	}
	log_verbose("Set format for sink %s: %u channel(s), %uHz, %s",
	    sink->name, format->channels, format->sampling_rate,
	    pcm_sample_format_str(format->sample_format));
	return EOK;
}

int audio_sink_remove_source(audio_sink_t *sink, audio_source_t *source)
{
	assert(sink);
	assert(source);
	assert(list_member(&source->link, &sink->sources));
	list_remove(&source->link);
	if (sink->connection_change) {
		const int ret = sink->connection_change(sink, false);
		if (ret != EOK) {
			log_debug("Connected hook failed.");
			list_append(&source->link, &sink->sources);
			return ret;
		}
	}
	audio_source_connected(source, NULL);
	return EOK;
}


void audio_sink_mix_inputs(audio_sink_t *sink, void* dest, size_t size)
{
	assert(sink);
	assert(dest);

	pcm_format_silence(dest, size, &sink->format);
	list_foreach(sink->sources, it) {
		audio_source_t *source = audio_source_list_instance(it);
		const int ret =
		    audio_source_add_self(source, dest, size, &sink->format);
		if (ret != EOK) {
			log_warning("Failed to mix source %s: %s",
			    source->name, str_error(ret));
		}
	}
}


/**
 * @}
 */
