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
#include <macros.h>

#include "hound_ctx.h"
#include "audio_data.h"
#include "connection.h"
#include "log.h"

static int update_data(audio_source_t *source, size_t size);

hound_ctx_t *hound_record_ctx_get(const char *name)
{
	return NULL;
}

hound_ctx_t *hound_playback_ctx_get(const char *name)
{
	hound_ctx_t *ctx = malloc(sizeof(hound_ctx_t));
	if (ctx) {
		link_initialize(&ctx->link);
		list_initialize(&ctx->streams);
		ctx->sink = NULL;
		ctx->source = malloc(sizeof(audio_source_t));
		if (!ctx->source) {
			free(ctx);
			return NULL;
		}
		const int ret = audio_source_init(ctx->source, name, ctx, NULL,
		    update_data, &AUDIO_FORMAT_DEFAULT);
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
	//TODO remove streams
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
	return ctx->source == NULL;
}


/*
 * STREAMS
 */
typedef struct hound_ctx_stream {
	link_t link;
	audio_pipe_t fifo;
	hound_ctx_t *ctx;
	pcm_format_t format;
	int flags;
	size_t allowed_size;
} hound_ctx_stream_t;

static inline hound_ctx_stream_t *hound_ctx_stream_from_link(link_t *l)
{
	return l ? list_get_instance(l, hound_ctx_stream_t, link) : NULL;
}

hound_ctx_stream_t *hound_ctx_create_stream(hound_ctx_t *ctx, int flags,
	pcm_format_t format, size_t buffer_size)
{
	assert(ctx);
	hound_ctx_stream_t *stream = malloc(sizeof(hound_ctx_stream_t));
	if (stream) {
		audio_pipe_init(&stream->fifo);
		link_initialize(&stream->link);
		stream->ctx = ctx;
		stream->flags = flags;
		stream->format = format;
		stream->allowed_size = buffer_size;
		list_append(&stream->link, &ctx->streams);
		log_verbose("CTX: %p added stream; flags:%#x ch: %u r:%u f:%s",
		    ctx, flags, format.channels, format.sampling_rate,
		    pcm_sample_format_str(format.sample_format));
	}
	return stream;
}

void hound_ctx_destroy_stream(hound_ctx_stream_t *stream)
{
	if (stream) {
		//TODO consider DRAIN FLAG
		list_remove(&stream->link);
		if (audio_pipe_bytes(&stream->fifo))
			log_warning("Destroying stream with non empty buffer");
		log_verbose("CTX: %p remove stream (%zu/%zu); "
		    "flags:%#x ch: %u r:%u f:%s",
		    stream->ctx, audio_pipe_bytes(&stream->fifo),
		    stream->allowed_size, stream->flags,
		    stream->format.channels, stream->format.sampling_rate,
		    pcm_sample_format_str(stream->format.sample_format));
		audio_pipe_fini(&stream->fifo);
		free(stream);
	}
}


int hound_ctx_stream_write(hound_ctx_stream_t *stream, const void *data,
    size_t size)
{
	assert(stream);
        log_verbose("%p: %zu", stream, size);

	if (stream->allowed_size && size > stream->allowed_size)
		return EINVAL;

	if (stream->allowed_size &&
	    (audio_pipe_bytes(&stream->fifo) + size > stream->allowed_size))
		return EBUSY;

	return audio_pipe_push_data(&stream->fifo, data, size, stream->format);
}

int hound_ctx_stream_read(hound_ctx_stream_t *stream, void *data, size_t size)
{
        log_verbose("%p:, %zu", stream, size);
	return ENOTSUP;
}

int hound_ctx_stream_add_self(hound_ctx_stream_t *stream, void *data,
    size_t size, const pcm_format_t *f)
{
	assert(stream);
	const ssize_t copied_size =
	    audio_pipe_mix_data(&stream->fifo, data, size, f);
	if (copied_size != (ssize_t)size)
		log_warning("Not enough data in stream buffer");
	return copied_size > 0 ? EOK : copied_size;
}

void hound_ctx_stream_drain(hound_ctx_stream_t *stream)
{
	assert(stream);
	log_debug("Draining stream");
	while (audio_pipe_bytes(&stream->fifo))
		async_usleep(10000);
}

int hound_ctx_stream_add(hound_ctx_stream_t *stream, void *buffer, size_t size,
    pcm_format_t format)
{
	return ENOTSUP;
}


int update_data(audio_source_t *source, size_t size)
{
	assert(source);
	assert(source->private_data);
	hound_ctx_t *ctx = source->private_data;
	void *buffer = malloc(size);
	if (!buffer)
		return ENOMEM;
	audio_data_t *adata = audio_data_create(buffer, size, source->format);
	if (!adata) {
		free(buffer);
		return ENOMEM;
	}
	log_verbose("CTX: %p. Mixing %zu streams", ctx,
	    list_count(&ctx->streams));
	pcm_format_silence(buffer, size, &source->format);
	list_foreach(ctx->streams, it) {
		hound_ctx_stream_t *stream = hound_ctx_stream_from_link(it);
		hound_ctx_stream_add_self(stream, buffer, size, &source->format);
	}
	log_verbose("CTX: %p. Pushing audio to %zu connections", ctx,
	    list_count(&source->connections));
	list_foreach(source->connections, it) {
		connection_t *conn = connection_from_source_list(it);
		connection_push_data(conn, adata);
	}
	return EOK;
}

/**
 * @}
 */
