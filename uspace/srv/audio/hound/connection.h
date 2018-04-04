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

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <assert.h>
#include <adt/list.h>
#include <pcm/format.h>

#include "audio_data.h"
#include "audio_source.h"
#include "audio_sink.h"

/** Source->sink connection structure */
typedef struct {
	/** Source's connections link */
	link_t source_link;
	/** Sink's connections link */
	link_t sink_link;
	/** Hound's connections link */
	link_t hound_link;
	/** audio data pipe */
	audio_pipe_t fifo;
	/** Target sink */
	audio_sink_t *sink;
	/** Target source */
	audio_source_t *source;
} connection_t;

/**
 * List instance helper.
 * @param l link
 * @return pointer to a connection structure, NULL on failure.
 */
static inline connection_t *connection_from_source_list(link_t *l)
{
	return l ? list_get_instance(l, connection_t, source_link) : NULL;
}

/**
 * List instance helper.
 * @param l link
 * @return pointer to a connection structure, NULL on failure.
 */
static inline connection_t *connection_from_hound_list(link_t *l)
{
	return l ? list_get_instance(l, connection_t, hound_link) : NULL;
}

connection_t *connection_create(audio_source_t *source, audio_sink_t *sink);
void connection_destroy(connection_t *connection);

errno_t connection_add_source_data(connection_t *connection, void *data,
    size_t size, pcm_format_t format);

errno_t connection_push_data(connection_t *connection, audio_data_t *adata);

/**
 * Source name getter.
 * @param connection Connection to the source.
 * @param valid string identifier, "no source" or "unnamed source" on failure.
 */
static inline const char *connection_source_name(connection_t *connection)
{
	assert(connection);
	if (connection->source && connection->source->name)
		return connection->source->name;
	// TODO assert?
	return connection->source ? "unnamed source" : "no source";
}

/**
 * Sink name getter.
 * @param connection Connection to the sink.
 * @param valid string identifier, "no source" or "unnamed source" on failure.
 */
static inline const char *connection_sink_name(connection_t *connection)
{
	assert(connection);
	if (connection->sink && connection->sink->name)
		return connection->sink->name;
	// TODO assert?
	return connection->source ? "unnamed sink" : "no sink";
}


#endif
/**
 * @}
 */
