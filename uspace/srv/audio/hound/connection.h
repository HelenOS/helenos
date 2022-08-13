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
