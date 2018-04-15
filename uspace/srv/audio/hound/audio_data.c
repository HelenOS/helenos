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
#include <stdlib.h>

#include "audio_data.h"
#include "log.h"

/**
 * Create reference counted buffer out of ordinary data buffer.
 * @param data audio buffer. The memory passed will be freed eventually.
 * @param size Size of the @p data buffer.
 * @param format audio data format.
 * @return pointer to valid audio data structure, NULL on failure.
 */
audio_data_t *audio_data_create(void *data, size_t size,
    pcm_format_t format)
{
	audio_data_t *adata = malloc(sizeof(audio_data_t) + size);
	if (adata) {
		unsigned overflow = size % pcm_format_frame_size(&format);
		if (overflow)
			log_warning("Data not a multiple of frame size, "
			    "clipping.");
		adata->data = data;
		adata->size = size - overflow;
		adata->format = format;
		atomic_set(&adata->refcount, 1);
	}
	return adata;
}

/**
 * Get a new reference to the audio data buffer.
 * @param adata The audio data buffer.
 */
void audio_data_addref(audio_data_t *adata)
{
	assert(adata);
	assert(atomic_get(&adata->refcount) > 0);
	atomic_inc(&adata->refcount);
}

/**
 * Release a reference to the audio data buffer.
 * @param adata The audio data buffer.
 */
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

/** Audio data buffer list helper structure. */
typedef struct {
	link_t link;
	audio_data_t *adata;
	size_t position;
} audio_data_link_t;

/** List instance helper function.
 * @param l link
 * @return valid pointer to data link structure, NULL on failure.
 */
static inline audio_data_link_t *audio_data_link_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_data_link_t, link) : NULL;
}

/**
 * Create a new audio data link.
 * @param adata Audio data to store.
 * @return Valid pointer to a new audio data link structure, NULL on failure.
 */
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

/**
 * Destroy audio data link.
 * @param link The link to destroy.
 *
 * Releases data reference.
 */
static void audio_data_link_destroy(audio_data_link_t *link)
{
	assert(link);
	assert(!link_in_use(&link->link));
	audio_data_unref(link->adata);
	free(link);
}

/**
 * Data link buffer start helper function.
 * @param alink audio data link
 * @return pointer to the beginning of data buffer.
 */
static inline const void *audio_data_link_start(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	return alink->adata->data + alink->position;
}

/**
 * Data link remaining size getter.
 * @param alink audio data link
 * @return Remaining size of valid data in the buffer.
 */
static inline size_t audio_data_link_remain_size(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	assert(alink->position <= alink->adata->size);
	return alink->adata->size - alink->position;
}


/**
 * Data link remaining frames getter.
 * @param alink audio data link
 * @return Number of remaining frames in the buffer.
 */
static inline size_t audio_data_link_available_frames(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	return pcm_format_size_to_frames(audio_data_link_remain_size(alink),
	    &alink->adata->format);
}

/* Audio Pipe */

/**
 * Initialize audio pipe structure.
 * @param pipe The pipe structure to initialize.
 */
void audio_pipe_init(audio_pipe_t *pipe)
{
	assert(pipe);
	list_initialize(&pipe->list);
	fibril_mutex_initialize(&pipe->guard);
	pipe->frames = 0;
	pipe->bytes = 0;
}

/**
 * Destroy all data in a pipe.
 * @param pipe The audio pipe to clean.
 */
void audio_pipe_fini(audio_pipe_t *pipe)
{
	assert(pipe);
	while (!list_empty(&pipe->list)) {
		audio_data_t *adata = audio_pipe_pop(pipe);
		audio_data_unref(adata);
	}
}

/**
 * Add new audio data to a pipe.
 * @param pipe The target pipe.
 * @param data The data.
 * @return Error code.
 */
errno_t audio_pipe_push(audio_pipe_t *pipe, audio_data_t *data)
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

/**
 * Retrieve data form a audio pipe.
 * @param pipe THe target pipe.
 * @return Valid pointer to audio data, NULL if the pipe was empty.
 */
audio_data_t *audio_pipe_pop(audio_pipe_t *pipe)
{
	assert(pipe);
	fibril_mutex_lock(&pipe->guard);
	audio_data_t *adata = NULL;
	link_t *l = list_first(&pipe->list);
	if (l) {
		audio_data_link_t *alink = audio_data_link_list_instance(l);
		list_remove(&alink->link);
		pipe->bytes -= audio_data_link_remain_size(alink);
		pipe->frames -= audio_data_link_available_frames(alink);
		adata = alink->adata;
		audio_data_addref(adata);
		audio_data_link_destroy(alink);
	}
	fibril_mutex_unlock(&pipe->guard);
	return adata;
}


/**
 * Use data store in a pipe and mix it into the provided buffer.
 * @param pipe The piep that should provide data.
 * @param data Target buffer.
 * @param size Target buffer size.
 * @param format Target data format.
 * @return Size of the target buffer used.
 */
size_t audio_pipe_mix_data(audio_pipe_t *pipe, void *data,
    size_t size, const pcm_format_t *f)
{
	assert(pipe);

	const size_t dst_frame_size = pcm_format_frame_size(f);
	size_t needed_frames = pcm_format_size_to_frames(size, f);
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
		const size_t dst_copy_size = copy_frames * dst_frame_size;
		const size_t src_copy_size = copy_frames * src_frame_size;

		assert(src_copy_size <= audio_data_link_remain_size(alink));

		/* Copy audio data */
		pcm_format_convert_and_mix(data, dst_copy_size,
		    audio_data_link_start(alink), src_copy_size,
		    &alink->adata->format, f);

		/* Update values */
		needed_frames -= copy_frames;
		copied_size += dst_copy_size;
		data += dst_copy_size;
		alink->position += src_copy_size;
		pipe->bytes -= src_copy_size;
		pipe->frames -= copy_frames;
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
