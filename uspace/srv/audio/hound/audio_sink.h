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

#ifndef AUDIO_SINK_H_
#define AUDIO_SINK_H_

#include <bool.h>
#include <adt/list.h>
#include <pcm_sample_format.h>
#include <fibril.h>

#include "audio_format.h"
#include "audio_source.h"

typedef struct audio_sink {
	link_t link;
	list_t sources;
	char *name;
	audio_format_t format;
	struct {
		int (*hook)(void* arg);
		void *arg;
	} connected_change;
} audio_sink_t;

static inline void audio_sink_set_connected_callback(audio_sink_t *sink,
    int (*hook)(void* arg), void* arg)
{
	assert(sink);
	sink->connected_change.arg = arg;
	sink->connected_change.hook = hook;
};
int audio_sink_init(audio_sink_t *sink, const char* name);
int audio_sink_add_source(audio_sink_t *sink, audio_source_t *source);
int audio_sink_remove_source(audio_sink_t *sink, audio_source_t *source);
void audio_sink_mix_inputs(audio_sink_t *sink, void* dest, size_t size);


#endif

/**
 * @}
 */

