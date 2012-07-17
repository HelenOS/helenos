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
#include <pcm/format.h>

struct audio_sink;
typedef struct audio_source audio_source_t;

struct audio_source {
	link_t link;
	const char *name;
	pcm_format_t format;
	void *private_data;
	int (*connection_change)(audio_source_t *source);
	int (*update_available_data)(audio_source_t *source, size_t size);
	struct audio_sink *connected_sink;
	struct {
		void *position;
		void *base;
		size_t size;
	} available_data;
};

static inline audio_source_t * audio_source_list_instance(link_t *l)
{
	return list_get_instance(l, audio_source_t, link);
}

int audio_source_init(audio_source_t *source, const char *name, void *data,
    int (*connection_change)(audio_source_t *),
    int (*update_available_data)(audio_source_t *, size_t),
    const pcm_format_t *f);
void audio_source_fini(audio_source_t *source);
int audio_source_connected(audio_source_t *source, struct audio_sink *sink);
int audio_source_add_self(audio_source_t *source, void *buffer, size_t size,
    const pcm_format_t *f);
static inline const pcm_format_t *audio_source_format(const audio_source_t *s)
{
	assert(s);
	return &s->format;
}
#endif

/**
 * @}
 */
