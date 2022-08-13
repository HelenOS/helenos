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
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "audio_sink.h"
#include "connection.h"
#include "log.h"

/**
 * Initialize audio sink structure.
 * @param sink The structure to initialize.
 * @param name string identifier
 * @param private_data backend data
 * @param connection_change connect/disconnect callback
 * @param check_format format testing callback
 * @param data_available trigger data prcoessing
 * @param f sink format
 * @return Error code.
 */
errno_t audio_sink_init(audio_sink_t *sink, const char *name, void *private_data,
    errno_t (*connection_change)(audio_sink_t *, bool),
    errno_t (*check_format)(audio_sink_t *), errno_t (*data_available)(audio_sink_t *),
    const pcm_format_t *f)
{
	assert(sink);
	if (!name)
		return EINVAL;
	link_initialize(&sink->link);
	list_initialize(&sink->connections);
	sink->name = str_dup(name);
	if (!sink->name)
		return ENOMEM;
	sink->private_data = private_data;
	sink->format = *f;
	sink->connection_change = connection_change;
	sink->check_format = check_format;
	sink->data_available = data_available;
	log_verbose("Initialized sink (%p) '%s'", sink, sink->name);
	return EOK;
}

/**
 * Release resources claimed by initialization.
 * @param sink the structure to release.
 */
void audio_sink_fini(audio_sink_t *sink)
{
	assert(sink);
	assert(list_empty(&sink->connections));
	assert(!sink->private_data);
	free(sink->name);
	sink->name = NULL;
}

/**
 * Set audio sink format and check with backend,
 * @param sink The target sink isntance.
 * @param format Th new format.
 * @return Error code.
 */
errno_t audio_sink_set_format(audio_sink_t *sink, const pcm_format_t *format)
{
	assert(sink);
	assert(format);
	if (!pcm_format_is_any(&sink->format)) {
		log_debug("Sink %s already has a format", sink->name);
		return EEXIST;
	}
	const pcm_format_t old_format = sink->format;

	if (pcm_format_is_any(format)) {
		log_verbose("Setting DEFAULT format for sink %s", sink->name);
		sink->format = AUDIO_FORMAT_DEFAULT;
	} else {
		sink->format = *format;
	}
	if (sink->check_format) {
		const errno_t ret = sink->check_format(sink);
		if (ret != EOK && ret != ELIMIT) {
			log_debug("Format check failed on sink %s", sink->name);
			sink->format = old_format;
			return ret;
		}
	}
	log_verbose("Set format for sink %s: %u channel(s), %uHz, %s",
	    sink->name, format->channels, format->sampling_rate,
	    pcm_sample_format_str(format->sample_format));
	return EOK;
}

/**
 * Pull data from all connections and add the mix to the destination.
 * @param sink The sink to mix data.
 * @param dest Destination buffer.
 * @param size size of the @p dest buffer.
 * @return Error code.
 */
void audio_sink_mix_inputs(audio_sink_t *sink, void *dest, size_t size)
{
	assert(sink);
	assert(dest);

	pcm_format_silence(dest, size, &sink->format);
	list_foreach(sink->connections, sink_link, connection_t, conn) {
		const errno_t ret = connection_add_source_data(
		    conn, dest, size, sink->format);
		if (ret != EOK) {
			log_warning("Failed to mix source %s: %s",
			    connection_source_name(conn), str_error(ret));
		}
	}
}

/**
 * @}
 */
