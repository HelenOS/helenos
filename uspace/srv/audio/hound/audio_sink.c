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

#if 0
static int loop(void* arg)
{
	audio_sink_t *sink = arg;
	assert(sink);
	while (sink->device) {
		while (sink->running) {
			//wait for usable buffer
			audio_sink_mix_inputs(sink,
			    sink->device->buffer.available_base,
			    sink->device->buffer.available_size);
		}
		//remove ready
		sleep(1);
	}
	return 0;
}
#endif


int audio_sink_init(audio_sink_t *sink, const char* name)
{
	assert(sink);
	if (!name) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	link_initialize(&sink->link);
	list_initialize(&sink->sources);
	sink->name = str_dup(name);
	sink->format = AUDIO_FORMAT_ANY;
	log_verbose("Initialized sink (%p) '%s' with ANY audio format",
	    sink, sink->name);
	return EOK;
}

int audio_sink_add_source(audio_sink_t *sink, audio_source_t *source)
{
	assert(sink);
	assert(source);
	assert_link_not_used(&source->link);
	list_append(&source->link, &sink->sources);

	const audio_format_t old_format = sink->format;

	/* The first source for me */
	if (list_count(&sink->sources) == 1) {
		/* Set audio format according to the first source */
		if (audio_format_is_any(&sink->format)) {
			/* Source does not care */
			if (audio_format_is_any(&source->format)) {
				log_verbose("Set default format for sink %s.",
				    sink->name);
				sink->format = AUDIO_FORMAT_DEFAULT;
			} else {
				log_verbose("Set format base on the first "
				    "source(%s): %u channels, %uHz, %s for "
				    " sink %s.", source->name,
				    source->format.channels,
				    source->format.sampling_rate,
				    pcm_sample_format_str(
				        source->format.sample_format),
				    sink->name);
				sink->format = source->format;
			}
		}
	}

	if (sink->connected_change.hook) {
		const int ret =
		    sink->connected_change.hook(sink->connected_change.arg);
		if (ret != EOK) {
			log_debug("Connected hook failed.");
			list_remove(&source->link);
			sink->format = old_format;
			return ret;
		}
	}

	return EOK;
}

int audio_sink_remove_source(audio_sink_t *sink, audio_source_t *source)
{
	assert(sink);
	assert(source);
	assert(list_member(&source->link, &sink->sources));
	list_remove(&source->link);
	if (sink->connected_change.hook) {
		const int ret =
		    sink->connected_change.hook(sink->connected_change.arg);
		if (ret != EOK) {
			log_debug("Connected hook failed.");
		}
	}
	return EOK;
}


void audio_sink_mix_inputs(audio_sink_t *sink, void* dest, size_t size)
{
	assert(sink);
	assert(dest);

	bzero(dest, size);
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
