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

/**
 * @addtogroup audio
 * @brief HelenOS sound server.
 * @{
 */
/** @file
 */

#include <assert.h>
#include <stdlib.h>
#include <str.h>

#include "hound.h"
#include "audio_device.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "connection.h"
#include "log.h"
#include "errno.h"
#include "str_error.h"

/**
 * Search devices by name.
 * @param name String identifier.
 * @return Pointer to the found device, NULL on failure.
 */
static audio_device_t *find_device_by_name(list_t *list, const char *name)
{
	assert(list);
	assert(name);

	list_foreach(*list, link, audio_device_t, dev) {
		if (str_cmp(name, dev->name) == 0) {
			log_debug("device with name '%s' is in the list",
			    name);
			return dev;
		}
	}

	return NULL;
}

/**
 * Search sources by name.
 * @param name String identifier.
 * @return Pointer to the found source, NULL on failure.
 */
static audio_source_t *find_source_by_name(list_t *list, const char *name)
{
	assert(list);
	assert(name);

	list_foreach(*list, link, audio_source_t, dev) {
		if (str_cmp(name, dev->name) == 0) {
			log_debug("source with name '%s' is in the list",
			    name);
			return dev;
		}
	}

	return NULL;
}

/**
 * Search sinks by name.
 * @param name String identifier.
 * @return Pointer to the found sink, NULL on failure.
 */
static audio_sink_t *find_sink_by_name(list_t *list, const char *name)
{
	assert(list);
	assert(name);

	list_foreach(*list, link, audio_sink_t, dev) {
		if (str_cmp(name, dev->name) == 0) {
			log_debug("sink with name '%s' is in the list",
			    name);
			return dev;
		}
	}

	return NULL;
}

static errno_t hound_disconnect_internal(hound_t *hound, const char *source_name, const char *sink_name);

/**
 * Remove provided sink.
 * @param hound The hound structure.
 * @param sink Target sink to remove.
 *
 * This function has to be called with the list_guard lock held.
 */
static void hound_remove_sink_internal(hound_t *hound, audio_sink_t *sink)
{
	assert(hound);
	assert(sink);
	assert(fibril_mutex_is_locked(&hound->list_guard));
	log_verbose("Removing sink '%s'.", sink->name);
	if (!list_empty(&sink->connections))
		log_warning("Removing sink '%s' while still connected.", sink->name);
	while (!list_empty(&sink->connections)) {
		connection_t *conn = list_get_instance(
		    list_first(&sink->connections), connection_t, sink_link);
		list_remove(&conn->hound_link);
		connection_destroy(conn);
	}
	list_remove(&sink->link);
}

/**
 * Remove provided source.
 * @param hound The hound structure.
 * @param sink Target source to remove.
 *
 * This function has to be called with the guard lock held.
 */
static void hound_remove_source_internal(hound_t *hound, audio_source_t *source)
{
	log_verbose("Removing source '%s'.", source->name);
	if (!list_empty(&source->connections))
		log_warning("Removing source '%s' while still connected.", source->name);
	while (!list_empty(&source->connections)) {
		connection_t *conn = list_get_instance(
		    list_first(&source->connections), connection_t, source_link);
		list_remove(&conn->hound_link);
		connection_destroy(conn);
	}
	list_remove(&source->link);
}

/**
 * Initialize hound structure.
 * @param hound The structure to initialize.
 * @return Error code.
 */
errno_t hound_init(hound_t *hound)
{
	assert(hound);
	fibril_mutex_initialize(&hound->list_guard);
	list_initialize(&hound->devices);
	list_initialize(&hound->contexts);
	list_initialize(&hound->sources);
	list_initialize(&hound->sinks);
	list_initialize(&hound->connections);
	return EOK;
}

/**
 * Add a new application context.
 * @param hound Hound structure.
 * @param ctx Context to add.
 * @return Error code.
 */
errno_t hound_add_ctx(hound_t *hound, hound_ctx_t *ctx)
{
	log_info("Trying to add context %p", ctx);
	assert(hound);
	if (!ctx)
		return EINVAL;
	fibril_mutex_lock(&hound->list_guard);
	list_append(&ctx->link, &hound->contexts);
	fibril_mutex_unlock(&hound->list_guard);
	errno_t ret = EOK;
	if (ret == EOK && ctx->source)
		ret = hound_add_source(hound, ctx->source);
	if (ret == EOK && ctx->sink)
		ret = hound_add_sink(hound, ctx->sink);
	if (ret != EOK) {
		fibril_mutex_lock(&hound->list_guard);
		list_remove(&ctx->link);
		fibril_mutex_unlock(&hound->list_guard);
	}
	return ret;
}

/**
 * Remove existing application context.
 * @param hound Hound structure.
 * @param ctx Context to remove.
 * @return Error code.
 */
errno_t hound_remove_ctx(hound_t *hound, hound_ctx_t *ctx)
{
	assert(hound);
	if (!ctx)
		return EINVAL;
	if (!list_empty(&ctx->streams))
		return EBUSY;
	fibril_mutex_lock(&hound->list_guard);
	list_remove(&ctx->link);
	if (ctx->source)
		hound_remove_source_internal(hound, ctx->source);
	if (ctx->sink)
		hound_remove_sink_internal(hound, ctx->sink);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * Search registered contexts for the matching id.
 * @param hound The hound structure.
 * @param id Requested id.
 * @return Pointer to the found structure, NULL on failure.
 */
hound_ctx_t *hound_get_ctx_by_id(hound_t *hound, hound_context_id_t id)
{
	assert(hound);

	fibril_mutex_lock(&hound->list_guard);
	hound_ctx_t *res = NULL;
	list_foreach(hound->contexts, link, hound_ctx_t, ctx) {
		if (hound_ctx_get_id(ctx) == id) {
			res = ctx;
			break;
		}
	}
	fibril_mutex_unlock(&hound->list_guard);
	return res;
}

/**
 * Add a new device.
 * @param hound The hound structure.
 * @param id Locations service id representing the device driver.
 * @param name String identifier.
 * @return Error code.
 */
errno_t hound_add_device(hound_t *hound, service_id_t id, const char *name)
{
	log_verbose("Adding device \"%s\", service: %zu", name, id);

	assert(hound);
	if (!name || !id) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}

	list_foreach(hound->devices, link, audio_device_t, dev) {
		if (dev->id == id) {
			log_debug("Device with id %zu is already present", id);
			return EEXIST;
		}
	}

	audio_device_t *dev = find_device_by_name(&hound->devices, name);
	if (dev) {
		log_debug("Device with name %s is already present", name);
		return EEXIST;
	}

	dev = malloc(sizeof(audio_device_t));
	if (!dev) {
		log_debug("Failed to malloc device structure.");
		return ENOMEM;
	}

	const errno_t ret = audio_device_init(dev, id, name);
	if (ret != EOK) {
		log_debug("Failed to initialize new audio device: %s",
		    str_error(ret));
		free(dev);
		return ret;
	}

	list_append(&dev->link, &hound->devices);
	log_info("Added new device: '%s'", dev->name);

	audio_source_t *source = audio_device_get_source(dev);
	if (source) {
		const errno_t ret = hound_add_source(hound, source);
		if (ret != EOK) {
			log_debug("Failed to add device source: %s",
			    str_error(ret));
			audio_device_fini(dev);
			return ret;
		}
		log_verbose("Added source: '%s'.", source->name);
	}

	audio_sink_t *sink = audio_device_get_sink(dev);
	if (sink) {
		const errno_t ret = hound_add_sink(hound, sink);
		if (ret != EOK) {
			log_debug("Failed to add device sink: %s",
			    str_error(ret));
			audio_device_fini(dev);
			return ret;
		}
		log_verbose("Added sink: '%s'.", sink->name);
	}

	if (!source && !sink)
		log_warning("Neither sink nor source on device '%s'.", name);

	return ret;
}

/**
 * Register a new source.
 * @param hound The hound structure.
 * @param source A new source to add.
 * @return Error code.
 */
errno_t hound_add_source(hound_t *hound, audio_source_t *source)
{
	assert(hound);
	if (!source || !source->name || str_cmp(source->name, "default") == 0) {
		log_debug("Invalid source specified.");
		return EINVAL;
	}
	fibril_mutex_lock(&hound->list_guard);
	if (find_source_by_name(&hound->sources, source->name)) {
		log_debug("Source by that name already exists");
		fibril_mutex_unlock(&hound->list_guard);
		return EEXIST;
	}
	list_append(&source->link, &hound->sources);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * Register a new sink.
 * @param hound The hound structure.
 * @param sink A new sink to add.
 * @return Error code.
 */
errno_t hound_add_sink(hound_t *hound, audio_sink_t *sink)
{
	assert(hound);
	if (!sink || !sink->name || str_cmp(sink->name, "default") == 0) {
		log_debug("Invalid source specified.");
		return EINVAL;
	}
	fibril_mutex_lock(&hound->list_guard);
	if (find_sink_by_name(&hound->sinks, sink->name)) {
		log_debug("Sink by that name already exists");
		fibril_mutex_unlock(&hound->list_guard);
		return EEXIST;
	}
	list_append(&sink->link, &hound->sinks);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * Remove a registered source.
 * @param hound The hound structure.
 * @param source A registered source to remove.
 * @return Error code.
 */
errno_t hound_remove_source(hound_t *hound, audio_source_t *source)
{
	assert(hound);
	if (!source)
		return EINVAL;
	fibril_mutex_lock(&hound->list_guard);
	hound_remove_source_internal(hound, source);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * Remove a registered sink.
 * @param hound The hound structure.
 * @param sink A registered sink to remove.
 * @return Error code.
 */
errno_t hound_remove_sink(hound_t *hound, audio_sink_t *sink)
{
	assert(hound);
	if (!sink)
		return EINVAL;
	fibril_mutex_lock(&hound->list_guard);
	hound_remove_sink_internal(hound, sink);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * List all registered sources.
 * @param[in] hound The hound structure.
 * @param[out] list List of the string identifiers.
 * @param[out] size Number of identifiers int he @p list.
 * @return Error code.
 */
errno_t hound_list_sources(hound_t *hound, char ***list, size_t *size)
{
	assert(hound);
	if (!list || !size)
		return EINVAL;

	fibril_mutex_lock(&hound->list_guard);
	const unsigned long count = list_count(&hound->sources);
	if (count == 0) {
		*list = NULL;
		*size = 0;
		fibril_mutex_unlock(&hound->list_guard);
		return EOK;
	}
	char **names = calloc(count, sizeof(char *));
	errno_t ret = names ? EOK : ENOMEM;
	for (unsigned long i = 0; i < count && ret == EOK; ++i) {
		link_t *slink = list_nth(&hound->sources, i);
		audio_source_t *source = audio_source_list_instance(slink);
		names[i] = str_dup(source->name);
		if (names[i])
			ret = ENOMEM;
	}
	if (ret == EOK) {
		*size = count;
		*list = names;
	} else {
		for (size_t i = 0; i < count; ++i)
			free(names[i]);
		free(names);
	}
	fibril_mutex_unlock(&hound->list_guard);
	return ret;
}

/**
 * List all registered sinks.
 * @param[in] hound The hound structure.
 * @param[out] list List of the string identifiers.
 * @param[out] size Number of identifiers int he @p list.
 * @return Error code.
 */
errno_t hound_list_sinks(hound_t *hound, char ***list, size_t *size)
{
	assert(hound);
	if (!list || !size)
		return EINVAL;

	fibril_mutex_lock(&hound->list_guard);
	const size_t count = list_count(&hound->sinks);
	if (count == 0) {
		*list = NULL;
		*size = 0;
		fibril_mutex_unlock(&hound->list_guard);
		return EOK;
	}
	char **names = calloc(count, sizeof(char *));
	errno_t ret = names ? EOK : ENOMEM;
	for (size_t i = 0; i < count && ret == EOK; ++i) {
		link_t *slink = list_nth(&hound->sinks, i);
		audio_sink_t *sink = audio_sink_list_instance(slink);
		names[i] = str_dup(sink->name);
		if (!names[i])
			ret = ENOMEM;
	}
	if (ret == EOK) {
		*size = count;
		*list = names;
	} else {
		for (size_t i = 0; i < count; ++i)
			free(names[i]);
		free(names);
	}
	fibril_mutex_unlock(&hound->list_guard);
	return ret;
}

/**
 * List all connections
 * @param[in] hound The hound structure.
 * @param[out] sources List of the source string identifiers.
 * @param[out] sinks List of the sinks string identifiers.
 * @param[out] size Number of identifiers int he @p list.
 * @return Error code.
 *
 * Lists include duplicit name entries. The order of entries is important,
 * identifiers with the same index are connected.
 */
errno_t hound_list_connections(hound_t *hound, const char ***sources,
    const char ***sinks, size_t *size)
{
	fibril_mutex_lock(&hound->list_guard);
	fibril_mutex_unlock(&hound->list_guard);
	return ENOTSUP;
}

/**
 * Create and register a new connection.
 * @param hound The hound structure.
 * @param source_name Source's string id.
 * @param sink_name Sink's string id.
 * @return Error code.
 */
errno_t hound_connect(hound_t *hound, const char *source_name, const char *sink_name)
{
	assert(hound);
	log_verbose("Connecting '%s' to '%s'.", source_name, sink_name);
	fibril_mutex_lock(&hound->list_guard);

	audio_source_t *source =
	    audio_source_list_instance(list_first(&hound->sources));
	if (str_cmp(source_name, "default") != 0)
		source = find_source_by_name(&hound->sources, source_name);

	audio_sink_t *sink =
	    audio_sink_list_instance(list_first(&hound->sinks));
	if (str_cmp(sink_name, "default") != 0)
		sink = find_sink_by_name(&hound->sinks, sink_name);

	if (!source || !sink) {
		fibril_mutex_unlock(&hound->list_guard);
		log_debug("Source (%p), or sink (%p) not found", source, sink);
		return ENOENT;
	}
	connection_t *conn = connection_create(source, sink);
	if (!conn) {
		fibril_mutex_unlock(&hound->list_guard);
		log_debug("Failed to create connection");
		return ENOMEM;
	}
	list_append(&conn->hound_link, &hound->connections);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

/**
 * Find and destroy connection between source and sink.
 * @param hound The hound structure.
 * @param source_name Source's string id.
 * @param sink_name Sink's string id.
 * @return Error code.
 */
errno_t hound_disconnect(hound_t *hound, const char *source_name, const char *sink_name)
{
	assert(hound);
	fibril_mutex_lock(&hound->list_guard);
	const errno_t ret = hound_disconnect_internal(hound, source_name, sink_name);
	fibril_mutex_unlock(&hound->list_guard);
	return ret;
}

/**
 * Internal disconnect helper.
 * @param hound The hound structure.
 * @param source_name Source's string id.
 * @param sink_name Sink's string id.
 * @return Error code.
 *
 * This function has to be called with the list_guard lock held.
 */
static errno_t hound_disconnect_internal(hound_t *hound, const char *source_name,
    const char *sink_name)
{
	assert(hound);
	assert(fibril_mutex_is_locked(&hound->list_guard));
	log_debug("Disconnecting '%s' to '%s'.", source_name, sink_name);

	list_foreach_safe(hound->connections, it, next) {
		connection_t *conn = connection_from_hound_list(it);
		if (str_cmp(connection_source_name(conn), source_name) == 0 ||
		    str_cmp(connection_sink_name(conn), sink_name) == 0) {
			log_debug("Removing %s -> %s", connection_source_name(conn),
			    connection_sink_name(conn));
			list_remove(it);
			connection_destroy(conn);
		}
	}

	return EOK;
}
/**
 * @}
 */
