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
#include <pcm/format.h>

typedef struct audio_source audio_source_t;

/** Audio data source abstraction structure */
struct audio_source {
	/** Link in hound's sources list */
	link_t link;
	/** List of connections */
	list_t connections;
	/** String identifier */
	char *name;
	/** audio data format */
	pcm_format_t format;
	/** backend data */
	void *private_data;
	/** Callback for connection and disconnection */
	errno_t (*connection_change)(audio_source_t *source, bool added);
	/** Ask backend for more data */
	errno_t (*update_available_data)(audio_source_t *source, size_t size);
};

/**
 * List instance helper.
 * @param l link
 * @return pointer to a source structure, NULL on failure.
 */
static inline audio_source_t *audio_source_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_source_t, link) : NULL;
}

errno_t audio_source_init(audio_source_t *source, const char *name, void *data,
    errno_t (*connection_change)(audio_source_t *, bool),
    errno_t (*update_available_data)(audio_source_t *, size_t),
    const pcm_format_t *f);
void audio_source_fini(audio_source_t *source);
errno_t audio_source_push_data(audio_source_t *source, void *data,
    size_t size);
static inline const pcm_format_t *audio_source_format(const audio_source_t *s)
{
	assert(s);
	return &s->format;
}
#endif

/**
 * @}
 */
