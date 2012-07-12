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

#ifndef AUDIO_SOURCE_H_
#define AUDIO_SOURCE_H_

#include <adt/list.h>
#include <bool.h>
#include <pcm_sample_format.h>

#include "audio_format.h"


typedef struct {
	link_t link;
	char *name;
	struct {
		int (*hook)(void* arg, const audio_format_t *f);
		void *arg;
	} connected_change;
	struct {
		int (*hook)(void* arg);
		void *arg;
	} get_data;
	struct {
		void *base;
		size_t size;
	} available;
	audio_format_t format;
} audio_source_t;

static inline audio_source_t * audio_source_list_instance(link_t *l)
{
	return list_get_instance(l, audio_source_t, link);
}

int audio_source_init(audio_source_t *source, const char *name);
static inline void audio_source_set_connected_callback(audio_source_t *source,
    int (*hook)(void* arg, const audio_format_t *f), void* arg)
{
	assert(source);
	source->connected_change.arg = arg;
	source->connected_change.hook = hook;
}
int audio_source_connected(audio_source_t *source, const audio_format_t *f);
int audio_source_get_buffer(audio_source_t *source, const void **buffer, size_t *size);
static inline const audio_format_t *audio_source_format(const audio_source_t *s)
{
	assert(s);
	return &s->format;
}
#endif

/**
 * @}
 */
