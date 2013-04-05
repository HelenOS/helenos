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

#include <macros.h>
#include <malloc.h>
#include "audio_data.h"

audio_data_t *audio_data_create(const void *data, size_t size,
    pcm_format_t format)
{
	audio_data_t *adata = malloc(sizeof(audio_data_t));
	if (adata) {
		adata->data = data;
		adata->size = size;
		adata->format = format;
		atomic_set(&adata->refcount, 1);
	}
	return adata;
}

void audio_data_addref(audio_data_t *adata)
{
	assert(adata);
	assert(atomic_get(&adata->refcount) > 0);
	atomic_inc(&adata->refcount);
}

void audio_data_unref(audio_data_t *adata)
{
	assert(adata);
	assert(atomic_get(&adata->refcount) > 0);
	atomic_count_t refc = atomic_predec(&adata->refcount);
	if (refc == 0) {
		free(adata->data);
		free(adata);
	}
}

/* Data link helpers */

typedef struct {
	link_t link;
	audio_data_t *adata;
	size_t position;
} audio_data_link_t;

static inline audio_data_link_t * audio_data_link_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_data_link_t, link) : NULL;
}

static audio_data_link_t *audio_data_link_create(audio_data_t *adata)
{
	assert(adata);
	audio_data_link_t *link = malloc(sizeof(audio_data_link_t));
	if (link) {
		audio_data_addref(adata);
		link->adata = adata;
		link->position = 0;
	}
	return link;
}

static void audio_data_link_destroy(audio_data_link_t *link)
{
	assert(link);
	assert(!link_in_use(&link->link));
	audio_data_unref(link->adata);
	free(link);
}

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


static inline size_t audio_data_link_available_frames(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	return pcm_format_size_to_frames(audio_data_link_remain_size(alink),
	    &alink->adata->format);
}

/* Audio Pipe */


void audio_pipe_init(audio_pipe_t *pipe)
{
	assert(pipe);
	list_initialize(&pipe->list);
	fibril_mutex_initialize(&pipe->guard);
	pipe->frames = 0;
	pipe->bytes = 0;
}



void audio_pipe_fini(audio_pipe_t *pipe)
{
	assert(pipe);
	while (!list_empty(&pipe->list)) {
		audio_data_t *adata = audio_pipe_pop(pipe);
		audio_data_unref(adata);
	}
}

int audio_pipe_push(audio_pipe_t *pipe, audio_data_t *data)
{
	assert(pipe);
	assert(data);
	audio_data_link_t *alink = audio_data_link_create(data);
	if (!alink)
		return ENOMEM;

	fibril_mutex_lock(&pipe->guard);
	list_append(&alink->link, &pipe->list);
	pipe->bytes += audio_data_link_remain_size(alink);
	pipe->frames += audio_data_link_available_frames(alink);
	fibril_mutex_unlock(&pipe->guard);
	return EOK;
}

audio_data_t *audio_pipe_pop(audio_pipe_t *pipe)
{
	assert(pipe);
	fibril_mutex_lock(&pipe->guard);
	audio_data_t *adata = NULL;
	link_t *l = list_first(&pipe->list);
	if (l) {
		audio_data_link_t *alink = audio_data_link_list_instance(l);
		list_remove(&alink->link);
		adata = alink->adata;
		audio_data_addref(adata);
		audio_data_link_destroy(alink);
	}
	fibril_mutex_unlock(&pipe->guard);
	return adata;
}

ssize_t audio_pipe_mix_data(audio_pipe_t *pipe, void *data,
    size_t size, const pcm_format_t *f)
{
	assert(pipe);
	const size_t dst_frame_size = pcm_format_frame_size(f);
	size_t needed_frames = size / dst_frame_size;
	size_t copied_size = 0;
	fibril_mutex_lock(&pipe->guard);
	while (needed_frames > 0 && !list_empty(&pipe->list)) {
		/* Get first audio chunk */
		link_t *l = list_first(&pipe->list);
		audio_data_link_t *alink = audio_data_link_list_instance(l);

		/* Get audio chunk metadata */
		const size_t src_frame_size =
		    pcm_format_frame_size(&alink->adata->format);
		const size_t available_frames =
		    audio_data_link_available_frames(alink);
		const size_t copy_frames = min(available_frames, needed_frames);
		const size_t copy_size = copy_frames * dst_frame_size;

		/* Copy audio data */
		pcm_format_convert_and_mix(data, copy_size,
		    audio_data_link_start(alink),
		    audio_data_link_remain_size(alink),
		    &alink->adata->format, f);

		/* Update values */
		copied_size += copy_size;
		needed_frames -= copy_frames;
		data += copy_size;
		alink->position += (copy_frames * src_frame_size);
		if (audio_data_link_remain_size(alink) == 0) {
			list_remove(&alink->link);
			audio_data_link_destroy(alink);
		} else {
			assert(needed_frames == 0);
		}
	}
	fibril_mutex_unlock(&pipe->guard);
	return copied_size;
}

/**
 * @}
 */
