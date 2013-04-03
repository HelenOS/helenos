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

#ifndef HOUND_CTX_H_
#define HOUND_CTX_H_

#include <adt/list.h>
#include <hound/protocol.h>
#include "audio_source.h"
#include "audio_sink.h"

typedef struct {
	link_t link;
	list_t streams;
	audio_source_t *source;
	audio_sink_t *sink;
} hound_ctx_t;

static inline hound_ctx_t *hound_ctx_from_link(link_t *l)
{
	return l ? list_get_instance(l, hound_ctx_t, link) : NULL;
}

hound_ctx_t *hound_record_ctx_get(const char *name);
hound_ctx_t *hound_playback_ctx_get(const char *name);
void hound_ctx_destroy(hound_ctx_t *context);

hound_context_id_t hound_ctx_get_id(hound_ctx_t *ctx);
bool hound_ctx_is_record(hound_ctx_t *ctx);

#endif

/**
 * @}
 */

