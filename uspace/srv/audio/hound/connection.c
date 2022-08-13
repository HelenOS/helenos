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

#include <errno.h>
#include <stdlib.h>

#include "log.h"
#include "connection.h"

/**
 * Create connection between source and sink.
 * @param source Valid source structure.
 * @param sink Valid sink structure.
 * @return pointer to a valid connection structure, NULL on failure.
 *
 * Reports new connection to both the source and sink.
 */
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

/**
 * Destroy existing connection
 * @param connection The connection to destroy.
 *
 * Disconnects from both the source and the sink.
 */
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

/**
 * Update and mix data provided by the source.
 * @param connection the connection to add.
 * @param data Destination audio buffer.
 * @param size size of the destination audio buffer.
 * @param format format of the destination audio buffer.
 */
errno_t connection_add_source_data(connection_t *connection, void *data,
    size_t size, pcm_format_t format)
{
	assert(connection);
	if (!data)
		return EBADMEM;
	const size_t needed_frames = pcm_format_size_to_frames(size, &format);
	if (needed_frames > audio_pipe_frames(&connection->fifo) &&
	    connection->source->update_available_data) {
		log_debug("Asking source to provide more data");
		connection->source->update_available_data(
		    connection->source, size);
	}
	log_verbose("Data available after update: %zu",
	    audio_pipe_bytes(&connection->fifo));
	size_t ret =
	    audio_pipe_mix_data(&connection->fifo, data, size, &format);
	if (ret != size)
		log_warning("Connection failed to provide enough data %zd/%zu",
		    ret, size);
	return EOK;
}
/**
 * Add new data to the connection buffer.
 * @param connection Target conneciton.
 * @aparam adata Reference counted audio data buffer.
 * @return Error code.
 */
errno_t connection_push_data(connection_t *connection,
    audio_data_t *adata)
{
	assert(connection);
	assert(adata);
	const errno_t ret = audio_pipe_push(&connection->fifo, adata);
	if (ret == EOK && connection->sink->data_available)
		connection->sink->data_available(connection->sink);
	return ret;
}

/**
 * @}
 */
