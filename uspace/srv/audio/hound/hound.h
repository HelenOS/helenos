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

#ifndef HOUND_H_
#define HOUND_H_

#include <async.h>
#include <adt/list.h>
#include <ipc/loc.h>
#include <errno.h>
#include <fibril_synch.h>
#include <pcm/format.h>
#include <hound/protocol.h>

#include "hound_ctx.h"
#include "audio_source.h"
#include "audio_sink.h"

/** The main Hound structure */
typedef struct {
	/** List access guard */
	fibril_mutex_t list_guard;
	/** enumerated devices */
	list_t devices;
	/** registered contexts */
	list_t contexts;
	/** Provided sources */
	list_t sources;
	/** Provided sinks */
	list_t sinks;
	/** Existing connections. */
	list_t connections;
} hound_t;

errno_t hound_init(hound_t *hound);
errno_t hound_add_ctx(hound_t *hound, hound_ctx_t *ctx);
errno_t hound_remove_ctx(hound_t *hound, hound_ctx_t *ctx);
hound_ctx_t *hound_get_ctx_by_id(hound_t *hound, hound_context_id_t id);

errno_t hound_add_device(hound_t *hound, service_id_t id, const char *name);
errno_t hound_add_source(hound_t *hound, audio_source_t *source);
errno_t hound_add_sink(hound_t *hound, audio_sink_t *sink);
errno_t hound_list_sources(hound_t *hound, char ***list, size_t *size);
errno_t hound_list_sinks(hound_t *hound, char ***list, size_t *size);
errno_t hound_list_connections(hound_t *hound, const char ***sources,
    const char ***sinks, size_t *size);
errno_t hound_remove_source(hound_t *hound, audio_source_t *source);
errno_t hound_remove_sink(hound_t *hound, audio_sink_t *sink);
errno_t hound_connect(hound_t *hound, const char *source_name, const char *sink_name);
errno_t hound_disconnect(hound_t *hound, const char *source_name, const char *sink_name);

#endif

/**
 * @}
 */
