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
#include <macros.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "audio_source.h"
#include "audio_sink.h"
#include "log.h"


int audio_source_init(audio_source_t *source, const char *name, void *data,
    int (*connection_change)(audio_source_t *),
    int (*update_available_data)(audio_source_t *, size_t),
    const audio_format_t *f)
{
	assert(source);
	if (!name || !f) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	link_initialize(&source->link);
	source->name = str_dup(name);
	source->private_data = data;
	source->connection_change = connection_change;
	source->update_available_data = update_available_data;
	source->connected_sink = NULL;
	source->format = *f;
	source->available_data.base = NULL;
	source->available_data.position = NULL;
	source->available_data.size = 0;
	log_verbose("Initialized source (%p) '%s'", source, source->name);
	return EOK;
}

void audio_source_fini(audio_source_t *source)
{
	assert(source);
	assert(source->connected_sink == NULL);
	free(source->name);
	source->name = NULL;
}

int audio_source_connected(audio_source_t *source, struct audio_sink *sink)
{
	assert(source);
	audio_sink_t *old_sink = source->connected_sink;
	const audio_format_t old_format = source->format;

	source->connected_sink = sink;
	if (audio_format_is_any(&source->format)) {
		assert(sink);
		assert(!audio_format_is_any(&sink->format));
		source->format = sink->format;
	}
	if (source->connection_change) {
		const int ret = source->connection_change(source);
		if (ret != EOK) {
			source->format = old_format;
			source->connected_sink = old_sink;
			return ret;
		}
	}
	return EOK;
}

int audio_source_add_self(audio_source_t *source, void *buffer, size_t size,
    const audio_format_t *f)
{
	assert(source);
	if (!buffer) {
		log_debug("Non-existent buffer");
		return EBADMEM;
	}
	if (!f || size == 0) {
		log_debug("No format or zero size");
	}
	if (size % (pcm_sample_format_size(f->sample_format) * f->channels)) {
		log_debug("Buffer does not fit integer number of frames");
		return EINVAL;
	}
	if (source->format.sampling_rate != f->sampling_rate) {
		log_debug("Resampling is not supported, yet");
		return ENOTSUP;
	}
	const size_t src_frame_size = audio_format_frame_size(&source->format);
	const size_t dst_frames = size / audio_format_frame_size(f);

	if (source->available_data.position == NULL ||
	    source->available_data.size == 0) {
		int ret = EOVERFLOW; /* In fact this is underflow... */
		if (source->update_available_data)
			ret = source->update_available_data(source,
			    dst_frames * src_frame_size);
		if (ret != EOK) {
			log_debug("No data to add to %p(%zu)", buffer, size);
			return ret;
		}
	}

	const int ret = audio_format_convert_and_mix(buffer, size,
	       source->available_data.position, source->available_data.size,
	       &source->format, f);
	if (ret != EOK) {
		log_debug("Mixing failed %p <= %p, frames: %zu",
		    buffer, source->available_data.position, dst_frames);
		return ret;
	}

	source->available_data.position += (dst_frames * src_frame_size);
	source->available_data.size -= (dst_frames * src_frame_size);
	return EOK;
}

/**
 * @}
 */
