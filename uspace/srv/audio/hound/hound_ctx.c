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

/**
 * @addtogroup audio
 * @brief HelenOS sound server.
 * @{
 */
/** @file
 */

#include <malloc.h>

#include "hound_ctx.h"

hound_ctx_t *hound_record_ctx_get(const char *name)
{
	return NULL;
}

hound_ctx_t *hound_playback_ctx_get(const char *name)
{
	hound_ctx_t *ctx = malloc(sizeof(hound_ctx_t));
	if (ctx) {
		link_initialize(&ctx->link);
		ctx->sink = NULL;
		ctx->source = malloc(sizeof(audio_source_t));
		if (!ctx->source) {
			free(ctx);
			return NULL;
		}
		const int ret = audio_source_init(ctx->source, name, ctx, NULL,
		    NULL, &AUDIO_FORMAT_ANY);
		if (ret != EOK) {
			free(ctx->source);
			free(ctx);
			return NULL;
		}
	}
	return ctx;
}

void hound_ctx_destroy(hound_ctx_t *ctx)
{
	assert(ctx);
	assert(!link_in_use(&ctx->link));
	if (ctx->source)
		audio_source_fini(ctx->source);
	if (ctx->sink)
		audio_sink_fini(ctx->sink);
	free(ctx->source);
	free(ctx->sink);
	free(ctx);
}

hound_context_id_t hound_ctx_get_id(hound_ctx_t *ctx)
{
	assert(ctx);
	return (hound_context_id_t)ctx;
}

bool hound_ctx_is_record(hound_ctx_t *ctx)
{
	assert(ctx);
	//TODO fix this
	return false;
}

/**
 * @}
 */
