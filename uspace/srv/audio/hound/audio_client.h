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

#ifndef AUDIO_CLIENT_H_
#define AUDIO_CLIENT_H_

#include <adt/list.h>
#include <async.h>
#include <pcm/format.h>

#include "audio_source.h"
#include "audio_sink.h"

typedef struct {
	link_t link;
	const char *name;
	audio_source_t source;
	audio_sink_t sink;
	pcm_format_t format;
	async_sess_t *sess;
	async_exch_t *exch;
	bool is_playback;
	bool is_recording;
} audio_client_t;

static inline audio_client_t * audio_client_list_instance(link_t *l)
{
	return list_get_instance(l, audio_client_t, link);
}

audio_client_t *audio_client_get_playback(
    const char *name, const pcm_format_t *f, async_sess_t *sess);
audio_client_t *audio_client_get_recording(
    const char *name, const pcm_format_t *f, async_sess_t *sess);
void audio_client_destroy(audio_client_t *client);

#endif
