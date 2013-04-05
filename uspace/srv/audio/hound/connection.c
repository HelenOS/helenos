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

#include <malloc.h>
#include <errno.h>

#include "log.h"
#include "connection.h"

connection_t *connection_create(audio_source_t *source, audio_sink_t *sink)
{
	assert(source);
	assert(sink);
	connection_t *conn = malloc(sizeof(connection_t));
	if (conn) {
		audio_pipe_init(&conn->fifo);
		link_initialize(&conn->source_link);
		link_initialize(&conn->sink_link);
		link_initialize(&conn->hound_link);
		conn->sink = sink;
		conn->source = source;
		list_append(&conn->source_link, &source->connections);
		list_append(&conn->sink_link, &sink->connections);
		audio_sink_set_format(sink, audio_source_format(source));
		if (source->connection_change)
			source->connection_change(source, true);
		if (sink->connection_change)
			sink->connection_change(sink, true);
		log_debug("CONNECTED: %s -> %s", source->name, sink->name);
	}
	return conn;
}

void connection_destroy(connection_t *connection)
{
	assert(connection);
	assert(!link_in_use(&connection->hound_link));
	list_remove(&connection->source_link);
	list_remove(&connection->sink_link);
	if (connection->sink && connection->sink->connection_change)
		connection->sink->connection_change(connection->sink, false);
	if (connection->source && connection->source->connection_change)
		connection->source->connection_change(connection->source, false);
	audio_pipe_fini(&connection->fifo);
	log_debug("DISCONNECTED: %s -> %s",
	    connection->source->name, connection->sink->name);
	free(connection);
}

ssize_t connection_add_source_data(connection_t *connection, void *data,
    size_t size, pcm_format_t format)
{
	assert(connection);
	if (!data)
		return EBADMEM;
	return audio_source_add_self(connection->source, data, size, &format);
}

/**
 * @}
 */
