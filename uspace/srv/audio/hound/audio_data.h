/*
 * Copyright (c) 2013 Jan Vesely
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

#ifndef AUDIO_DATA_H_
#define AUDIO_DATA_H_

#include <adt/list.h>
#include <atomic.h>
#include <errno.h>
#include <fibril_synch.h>
#include <pcm/format.h>

typedef struct {
	const void *data;
	size_t size;
	pcm_format_t format;
	atomic_t refcount;
} audio_data_t;

typedef struct {
	link_t link;
	audio_data_t *adata;
	size_t position;
} audio_data_link_t;

typedef struct {
	list_t list;
	size_t bytes;
	size_t frames;
	fibril_mutex_t guard;
} audio_pipe_t;

static inline audio_data_link_t * audio_data_link_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_data_link_t, link) : NULL;
}

audio_data_t * audio_data_create(const void *data, size_t size,
    pcm_format_t format);
void audio_data_addref(audio_data_t *adata);
void audio_data_unref(audio_data_t *adata);

audio_data_link_t * audio_data_link_create_data(const void *data, size_t size,
    pcm_format_t format);
audio_data_link_t *audio_data_link_create(audio_data_t *adata);
void audio_data_link_destroy(audio_data_link_t *link);

size_t audio_data_link_available_frames(audio_data_link_t *alink);
static inline const void * audio_data_link_start(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	return alink->adata->data + alink->position;
}

static inline size_t audio_data_link_remain_size(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	assert(alink->position <= alink->adata->size);
	return alink->adata->size - alink->position;
}

void audio_pipe_init(audio_pipe_t *pipe);
void audio_pipe_fini(audio_pipe_t *pipe);

int audio_pipe_push(audio_pipe_t *pipe, audio_data_t *data);
audio_data_t *audio_pipe_pop(audio_pipe_t *pipe);

ssize_t audio_pipe_mix_data(audio_pipe_t *pipe, void *buffer, size_t size,
    const pcm_format_t *f);

static inline size_t audio_pipe_bytes(audio_pipe_t *pipe)
{
	assert(pipe);
	return pipe->bytes;
}

static inline size_t audio_pipe_frames(audio_pipe_t *pipe)
{
	assert(pipe);
	return pipe->frames;
}

static inline int audio_pipe_push_data(audio_pipe_t *pipe,
    const void *data, size_t size, pcm_format_t f)
{
	audio_data_t *adata = audio_data_create(data, size, f);
	if (adata) {
		const int ret = audio_pipe_push(pipe, adata);
		audio_data_unref(adata);
		return ret;
	}
	return ENOMEM;
}


#endif

/**
 * @}
 */
