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

#include <macros.h>
#include <errno.h>
#include <stdlib.h>
#include <str_error.h>

#include "hound_ctx.h"
#include "audio_data.h"
#include "connection.h"
#include "log.h"

static errno_t update_data(audio_source_t *source, size_t size);
static errno_t new_data(audio_sink_t *sink);

/**
 * Allocate and initialize hound context structure.
 * @param name String identifier.
 * @return Pointer to a new context structure, NULL on failure
 *
 * Creates record context structure.
 */
hound_ctx_t *hound_record_ctx_get(const char *name)
{
	hound_ctx_t *ctx = malloc(sizeof(hound_ctx_t));
	if (ctx) {
		link_initialize(&ctx->link);
		list_initialize(&ctx->streams);
		fibril_mutex_initialize(&ctx->guard);
		ctx->source = NULL;
		ctx->sink = malloc(sizeof(audio_sink_t));
		if (!ctx->sink) {
			free(ctx);
			return NULL;
		}
		const errno_t ret = audio_sink_init(ctx->sink, name, ctx, NULL,
		    NULL, new_data, &AUDIO_FORMAT_DEFAULT);
		if (ret != EOK) {
			free(ctx->sink);
			free(ctx);
			return NULL;
		}
	}
	return ctx;
}

/**
 * Allocate and initialize hound context structure.
 * @param name String identifier.
 * @return Pointer to a new context structure, NULL on failure
 *
 * Creates record context structure.
 */
hound_ctx_t *hound_playback_ctx_get(const char *name)
{
	hound_ctx_t *ctx = malloc(sizeof(hound_ctx_t));
	if (ctx) {
		link_initialize(&ctx->link);
		list_initialize(&ctx->streams);
		fibril_mutex_initialize(&ctx->guard);
		ctx->sink = NULL;
		ctx->source = malloc(sizeof(audio_source_t));
		if (!ctx->source) {
			free(ctx);
			return NULL;
		}
		const errno_t ret = audio_source_init(ctx->source, name, ctx, NULL,
		    update_data, &AUDIO_FORMAT_DEFAULT);
		if (ret != EOK) {
			free(ctx->source);
			free(ctx);
			return NULL;
		}
	}
	return ctx;
}

/**
 * Destroy existing context structure.
 * @param ctx hound cotnext to destroy.
 */
void hound_ctx_destroy(hound_ctx_t *ctx)
{
	assert(ctx);
	assert(!link_in_use(&ctx->link));
	assert(list_empty(&ctx->streams));
	if (ctx->source)
		audio_source_fini(ctx->source);
	if (ctx->sink)
		audio_sink_fini(ctx->sink);
	free(ctx->source);
	free(ctx->sink);
	free(ctx);
}

/**
 * Retrieve associated context id.
 * @param ctx hound context.
 * @return context id of the context.
 */
hound_context_id_t hound_ctx_get_id(hound_ctx_t *ctx)
{
	assert(ctx);
	return (hound_context_id_t)ctx;
}

/**
 * Query playback/record status of a hound context.
 * @param ctx Hound context.
 * @return True if ctx  is a recording context.
 */
bool hound_ctx_is_record(hound_ctx_t *ctx)
{
	assert(ctx);
	return ctx->source == NULL;
}

/*
 * STREAMS
 */

/** Hound stream structure. */
typedef struct hound_ctx_stream {
	/** Hound context streams link */
	link_t link;
	/** Audio data pipe */
	audio_pipe_t fifo;
	/** Parent context */
	hound_ctx_t *ctx;
	/** Stream data format */
	pcm_format_t format;
	/** Stream modifiers */
	int flags;
	/** Maximum allowed buffer size */
	size_t allowed_size;
	/** Fifo access synchronization */
	fibril_mutex_t guard;
	/** buffer status change condition */
	fibril_condvar_t change;
} hound_ctx_stream_t;

/**
 * New stream append helper.
 * @param ctx hound context.
 * @param stream A new stream.
 */
static inline void stream_append(hound_ctx_t *ctx, hound_ctx_stream_t *stream)
{
	assert(ctx);
	assert(stream);
	fibril_mutex_lock(&ctx->guard);
	list_append(&stream->link, &ctx->streams);
	if (list_count(&ctx->streams) == 1) {
		if (ctx->source && list_count(&ctx->source->connections) == 0)
			ctx->source->format = stream->format;
	}
	fibril_mutex_unlock(&ctx->guard);
}

/**
 * Push new data to stream, do not block.
 * @param stream The target stream.
 * @param adata The new data.
 * @return Error code.
 */
static errno_t stream_push_data(hound_ctx_stream_t *stream, audio_data_t *adata)
{
	assert(stream);
	assert(adata);

	if (stream->allowed_size && adata->size > stream->allowed_size)
		return EINVAL;

	fibril_mutex_lock(&stream->guard);
	if (stream->allowed_size &&
	    (audio_pipe_bytes(&stream->fifo) + adata->size >
	    stream->allowed_size)) {
		fibril_mutex_unlock(&stream->guard);
		return EOVERFLOW;

	}

	const errno_t ret = audio_pipe_push(&stream->fifo, adata);
	fibril_mutex_unlock(&stream->guard);
	if (ret == EOK)
		fibril_condvar_signal(&stream->change);
	return ret;
}

/**
 * Old stream remove helper.
 * @param ctx hound context.
 * @param stream An old stream.
 */
static inline void stream_remove(hound_ctx_t *ctx, hound_ctx_stream_t *stream)
{
	assert(ctx);
	assert(stream);
	fibril_mutex_lock(&ctx->guard);
	list_remove(&stream->link);
	fibril_mutex_unlock(&ctx->guard);
}

/**
 * Create new stream.
 * @param ctx Assocaited hound context.
 * @param flags Stream modidfiers.
 * @param format PCM data format.
 * @param buffer_size Maximum allowed buffer size.
 * @return Pointer to a new stream structure, NULL on failure.
 */
hound_ctx_stream_t *hound_ctx_create_stream(hound_ctx_t *ctx, int flags,
    pcm_format_t format, size_t buffer_size)
{
	assert(ctx);
	hound_ctx_stream_t *stream = malloc(sizeof(hound_ctx_stream_t));
	if (stream) {
		audio_pipe_init(&stream->fifo);
		link_initialize(&stream->link);
		fibril_mutex_initialize(&stream->guard);
		fibril_condvar_initialize(&stream->change);
		stream->ctx = ctx;
		stream->flags = flags;
		stream->format = format;
		stream->allowed_size = buffer_size;
		stream_append(ctx, stream);
		log_verbose("CTX: %p added stream; flags:%#x ch: %u r:%u f:%s",
		    ctx, flags, format.channels, format.sampling_rate,
		    pcm_sample_format_str(format.sample_format));
	}
	return stream;
}

/**
 * Destroy existing stream structure.
 * @param stream The stream to destroy.
 *
 * The function will print warning if there are data in the buffer.
 */
void hound_ctx_destroy_stream(hound_ctx_stream_t *stream)
{
	if (stream) {
		stream_remove(stream->ctx, stream);
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

/**
 * Write new data to a stream.
 * @param stream The destination stream.
 * @param data audio data buffer.
 * @param size size of the @p data buffer.
 * @return Error code.
 */
errno_t hound_ctx_stream_write(hound_ctx_stream_t *stream, void *data,
    size_t size)
{
	assert(stream);

	if (stream->allowed_size && size > stream->allowed_size)
		return EINVAL;

	fibril_mutex_lock(&stream->guard);
	while (stream->allowed_size &&
	    (audio_pipe_bytes(&stream->fifo) + size > stream->allowed_size)) {
		fibril_condvar_wait(&stream->change, &stream->guard);

	}

	const errno_t ret =
	    audio_pipe_push_data(&stream->fifo, data, size, stream->format);
	fibril_mutex_unlock(&stream->guard);
	if (ret == EOK)
		fibril_condvar_signal(&stream->change);
	return ret;
}

/**
 * Read data from a buffer.
 * @param stream The source buffer.
 * @param data Destination data buffer.
 * @param size Size of the @p data buffer.
 * @return Error code.
 */
errno_t hound_ctx_stream_read(hound_ctx_stream_t *stream, void *data, size_t size)
{
	assert(stream);

	if (stream->allowed_size && size > stream->allowed_size)
		return EINVAL;

	fibril_mutex_lock(&stream->guard);
	while (audio_pipe_bytes(&stream->fifo) < size) {
		fibril_condvar_wait(&stream->change, &stream->guard);
	}

	pcm_format_silence(data, size, &stream->format);
	const size_t ret =
	    audio_pipe_mix_data(&stream->fifo, data, size, &stream->format);
	fibril_mutex_unlock(&stream->guard);
	if (ret > 0) {
		fibril_condvar_signal(&stream->change);
		return EOK;
	}
	return EEMPTY;
}

/**
 * Add (mix) stream data to the destination buffer.
 * @param stream The source stream.
 * @param data Destination audio buffer.
 * @param size Size of the @p data buffer.
 * @param format Destination data format.
 * @return Size of the destination buffer touch with stream's data.
 */
size_t hound_ctx_stream_add_self(hound_ctx_stream_t *stream, void *data,
    size_t size, const pcm_format_t *f)
{
	assert(stream);
	fibril_mutex_lock(&stream->guard);
	const size_t ret = audio_pipe_mix_data(&stream->fifo, data, size, f);
	fibril_condvar_signal(&stream->change);
	fibril_mutex_unlock(&stream->guard);
	return ret;
}

/**
 * Block until the stream's buffer is empty.
 * @param stream Target stream.
 */
void hound_ctx_stream_drain(hound_ctx_stream_t *stream)
{
	assert(stream);
	log_debug("Draining stream");
	fibril_mutex_lock(&stream->guard);
	while (audio_pipe_bytes(&stream->fifo))
		fibril_condvar_wait(&stream->change, &stream->guard);
	fibril_mutex_unlock(&stream->guard);
}

/**
 * Update context data.
 * @param source Source abstraction.
 * @param size Required size in source's format.
 * @return error code.
 *
 * Mixes data from all streams and pushes it to all connections.
 */
errno_t update_data(audio_source_t *source, size_t size)
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
	log_verbose("CTX: %p: Mixing %zu streams", ctx,
	    list_count(&ctx->streams));
	pcm_format_silence(buffer, size, &source->format);
	fibril_mutex_lock(&ctx->guard);
	list_foreach(ctx->streams, link, hound_ctx_stream_t, stream) {
		ssize_t copied = hound_ctx_stream_add_self(
		    stream, buffer, size, &source->format);
		if (copied != (ssize_t)size)
			log_warning("Not enough data in stream buffer");
	}
	log_verbose("CTX: %p. Pushing audio to %zu connections", ctx,
	    list_count(&source->connections));
	list_foreach(source->connections, source_link, connection_t, conn) {
		connection_push_data(conn, adata);
	}
	/* all connections should now have their refs */
	audio_data_unref(adata);
	fibril_mutex_unlock(&ctx->guard);
	return EOK;
}

errno_t new_data(audio_sink_t *sink)
{
	assert(sink);
	assert(sink->private_data);
	hound_ctx_t *ctx = sink->private_data;

	fibril_mutex_lock(&ctx->guard);

	/* count available data */
	size_t available_frames = -1;  /* this is ugly.... */
	list_foreach(sink->connections, source_link, connection_t, conn) {
		available_frames = min(available_frames,
		    audio_pipe_frames(&conn->fifo));
	}

	const size_t bsize =
	    available_frames * pcm_format_frame_size(&sink->format);
	void *buffer = malloc(bsize);
	if (!buffer) {
		fibril_mutex_unlock(&ctx->guard);
		return ENOMEM;
	}
	audio_data_t *adata = audio_data_create(buffer, bsize, sink->format);
	if (!adata) {
		fibril_mutex_unlock(&ctx->guard);
		free(buffer);
		return ENOMEM;
	}

	/* mix data */
	pcm_format_silence(buffer, bsize, &sink->format);
	list_foreach(sink->connections, source_link, connection_t, conn) {
		/* This should not trigger data update on the source */
		connection_add_source_data(
		    conn, buffer, bsize, sink->format);
	}
	/* push to all streams */
	list_foreach(ctx->streams, link, hound_ctx_stream_t, stream) {
		const errno_t ret = stream_push_data(stream, adata);
		if (ret != EOK)
			log_error("Failed to push data to stream: %s",
			    str_error(ret));
	}
	audio_data_unref(adata);
	fibril_mutex_unlock(&ctx->guard);
	return ENOTSUP;
}

/**
 * @}
 */
