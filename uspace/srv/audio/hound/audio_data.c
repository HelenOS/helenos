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

#include <malloc.h>
#include "audio_data.h"

static void ref_inc(audio_data_t *adata)
{
	assert(adata);
	assert(atomic_get(&adata->refcount) > 0);
	atomic_inc(&adata->refcount);
}

static void ref_dec(audio_data_t *adata)
{
	assert(adata);
	assert(atomic_get(&adata->refcount) > 0);
	atomic_count_t refc = atomic_predec(&adata->refcount);
	if (refc == 0) {
		free(adata->data);
		free(adata);
	}
}

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

void audio_data_unref(audio_data_t *adata)
{
	ref_dec(adata);
}

audio_data_link_t *audio_data_link_create(audio_data_t *adata)
{
	assert(adata);
	audio_data_link_t *link = malloc(sizeof(audio_data_link_t));
	if (link) {
		ref_inc(adata);
		link->adata = adata;
		link->position = 0;
	}
	return link;
}

audio_data_link_t * audio_data_link_create_data(const void *data, size_t size,
    pcm_format_t format)
{
	audio_data_link_t *link = NULL;
	audio_data_t *adata = audio_data_create(data, size, format);
	if (adata) {
		link = audio_data_link_create(adata);
		/* This will either return refcount to 1 or clean adata if
		 * cloning failed */
		audio_data_unref(adata);
	}
	return link;
}

void audio_data_link_destroy(audio_data_link_t *link)
{
	assert(link);
	assert(!link_in_use(&link->link));
	ref_dec(link->adata);
	free(link);
}

size_t audio_data_link_available_frames(audio_data_link_t *alink)
{
	assert(alink);
	assert(alink->adata);
	return pcm_format_size_to_frames(alink->adata->size - alink->position,
	    &alink->adata->format);
}

/**
 * @}
 */
