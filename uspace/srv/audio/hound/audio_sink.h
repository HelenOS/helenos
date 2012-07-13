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
#include <bool.h>
#include <fibril.h>

#include "audio_source.h"
#include "audio_format.h"

typedef struct audio_sink audio_sink_t;

struct audio_sink {
	link_t link;
	list_t sources;
	const char *name;
	audio_format_t format;
	void *private_data;
	int (*connection_change)(audio_sink_t *, bool);
};

static inline audio_sink_t * audio_sink_list_instance(link_t *l)
{
	return list_get_instance(l, audio_sink_t, link);
}

int audio_sink_init(audio_sink_t *sink, const char *name,
    void *private_data, int (*connection_change)(audio_sink_t *, bool),
    const audio_format_t *f);
void audio_sink_fini(audio_sink_t *sink);

int audio_sink_add_source(audio_sink_t *sink, audio_source_t *source);
int audio_sink_remove_source(audio_sink_t *sink, audio_source_t *source);
void audio_sink_mix_inputs(audio_sink_t *sink, void* dest, size_t size);


#endif

/**
 * @}
 */

