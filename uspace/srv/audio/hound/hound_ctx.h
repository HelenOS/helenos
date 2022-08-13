/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <fibril_synch.h>

#include "audio_source.h"
#include "audio_sink.h"

/** Application context structure */
typedef struct {
	/** Hound's contexts link */
	link_t link;
	/** List of streams */
	list_t streams;
	/** Provided audio source abstraction */
	audio_source_t *source;
	/** Provided audio sink abstraction */
	audio_sink_t *sink;
	/** List access synchronization */
	fibril_mutex_t guard;
} hound_ctx_t;

typedef struct hound_ctx_stream hound_ctx_stream_t;

hound_ctx_t *hound_record_ctx_get(const char *name);
hound_ctx_t *hound_playback_ctx_get(const char *name);
void hound_ctx_destroy(hound_ctx_t *context);

hound_context_id_t hound_ctx_get_id(hound_ctx_t *ctx);
bool hound_ctx_is_record(hound_ctx_t *ctx);

hound_ctx_stream_t *hound_ctx_create_stream(hound_ctx_t *ctx, int flags,
    pcm_format_t format, size_t buffer_size);
void hound_ctx_destroy_stream(hound_ctx_stream_t *stream);

errno_t hound_ctx_stream_write(hound_ctx_stream_t *stream, void *buffer,
    size_t size);
errno_t hound_ctx_stream_read(hound_ctx_stream_t *stream, void *buffer, size_t size);
size_t hound_ctx_stream_add_self(hound_ctx_stream_t *stream, void *data,
    size_t size, const pcm_format_t *f);
void hound_ctx_stream_drain(hound_ctx_stream_t *stream);

#endif

/**
 * @}
 */
