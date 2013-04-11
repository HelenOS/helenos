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

#include <adt/list.h>
#include <async.h>
#include <stdbool.h>
#include <fibril.h>
#include <pcm/format.h>

#include "audio_source.h"

typedef struct audio_sink audio_sink_t;

/** Audio sink abstraction structure */
struct audio_sink {
	/** Link in hound's sink list */
	link_t link;
	/** List of all related connections */
	list_t connections;
	/** Sink's name */
	const char *name;
	/** Consumes data in this format */
	pcm_format_t format;
	/** Backend data */
	void *private_data;
	/** Connect/disconnect callback */
	int (*connection_change)(audio_sink_t *, bool);
	/** Backend callback to check data */
	int (*check_format)(audio_sink_t *);
	/** new data notifier */
	int (*data_available)(audio_sink_t *);
};

/**
 * List instance helper.
 * @param l link
 * @return pointer to a sink structure, NULL on failure.
 */
static inline audio_sink_t * audio_sink_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_sink_t, link) : NULL;
}

int audio_sink_init(audio_sink_t *sink, const char *name, void *private_data,
    int (*connection_change)(audio_sink_t *, bool),
    int (*check_format)(audio_sink_t *), int (*data_available)(audio_sink_t *),
    const pcm_format_t *f);

void audio_sink_fini(audio_sink_t *sink);
int audio_sink_set_format(audio_sink_t *sink, const pcm_format_t *format);
void audio_sink_mix_inputs(audio_sink_t *sink, void* dest, size_t size);

#endif
/**
 * @}
 */
