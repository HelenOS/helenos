/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <macros.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "audio_data.h"
#include "audio_source.h"
#include "audio_sink.h"
#include "connection.h"
#include "log.h"

/**
 * Initialize audio source strcture.
 * @param source The structure to initialize.
 * @param name String identifier of the audio source.
 * @param data Backend data.
 * @param connection_change Connect/disconnect callback.
 * @param update_available_data Data request callback.
 * @return Error code.
 */
errno_t audio_source_init(audio_source_t *source, const char *name, void *data,
    errno_t (*connection_change)(audio_source_t *, bool new),
    errno_t (*update_available_data)(audio_source_t *, size_t),
    const pcm_format_t *f)
{
	assert(source);
	if (!name || !f) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	link_initialize(&source->link);
	list_initialize(&source->connections);
	source->name = str_dup(name);
	if (!source->name)
		return ENOMEM;
	source->private_data = data;
	source->connection_change = connection_change;
	source->update_available_data = update_available_data;
	source->format = *f;
	log_verbose("Initialized source (%p) '%s'", source, source->name);
	return EOK;
}

/**
 * Release resources claimed by initialization.
 * @param source The structure to cleanup.
 */
void audio_source_fini(audio_source_t *source)
{
	assert(source);
	free(source->name);
	source->name = NULL;
}
/**
 * Push data to all connections.
 * @param source The source of the data.
 * @param dest Destination buffer.
 * @param size size of the @p dest buffer.
 * @return Error code.
 */
errno_t audio_source_push_data(audio_source_t *source, void *data,
    size_t size)
{
	assert(source);
	assert(data);

	audio_data_t *adata = audio_data_create(data, size, source->format);
	if (!adata)
		return ENOMEM;

	list_foreach(source->connections, source_link, connection_t, conn) {
		const errno_t ret = connection_push_data(conn, adata);
		if (ret != EOK) {
			log_warning("Failed push data to %s: %s",
			    connection_sink_name(conn), str_error(ret));
		}
	}
	audio_data_unref(adata);
	return EOK;
}

/**
 * @}
 */
