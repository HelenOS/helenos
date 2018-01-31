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
