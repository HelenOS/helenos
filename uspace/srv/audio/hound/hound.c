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

#include "hound.h"
#include "audio_client.h"
#include "audio_device.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "log.h"
#include "errno.h"
#include "str_error.h"

#define FIND_BY_NAME(type) \
do { \
	assert(list); \
	assert(name); \
	list_foreach(*list, it) { \
		audio_ ## type ## _t *dev = \
		    audio_ ## type ## _list_instance(it); \
		if (str_cmp(name, dev->name) == 0) { \
			log_debug("%s with name '%s' is in the list", \
			    #type, name); \
			return dev; \
		} \
	} \
	return NULL; \
} while (0)

static audio_device_t * find_device_by_name(list_t *list, const char *name)
{
	FIND_BY_NAME(device);
}
static audio_source_t * find_source_by_name(list_t *list, const char *name)
{
	FIND_BY_NAME(source);
}
static audio_sink_t * find_sink_by_name(list_t *list, const char *name)
{
	FIND_BY_NAME(sink);
}
static int hound_disconnect_internal(hound_t *hound, const char* source_name, const char* sink_name);

int hound_init(hound_t *hound)
{
	assert(hound);
	fibril_mutex_initialize(&hound->list_guard);
	list_initialize(&hound->devices);
	list_initialize(&hound->sources);
	list_initialize(&hound->sinks);
	return EOK;
}

int hound_add_device(hound_t *hound, service_id_t id, const char *name)
{
	log_verbose("Adding device \"%s\", service: %zu", name, id);

	assert(hound);
	if (!name || !id) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}

	list_foreach(hound->devices, it) {
		audio_device_t *dev = audio_device_list_instance(it);
		if (dev->id == id) {
			log_debug("Device with id %zu is already present", id);
			return EEXISTS;
		}
	}

	audio_device_t *dev = find_device_by_name(&hound->devices, name);
	if (dev) {
		log_debug("Device with name %s is already present", name);
		return EEXISTS;
	}

	dev = malloc(sizeof(audio_device_t));
	if (!dev) {
		log_debug("Failed to malloc device structure.");
		return ENOMEM;
	}

	const int ret = audio_device_init(dev, id, name);
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
		const int ret = hound_add_source(hound, source);
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
		const int ret = hound_add_sink(hound, sink);
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

int hound_add_source(hound_t *hound, audio_source_t *source)
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
		return EEXISTS;
	}
	list_foreach(hound->sinks, it) {
		audio_sink_t *sink = audio_sink_list_instance(it);
		if (find_source_by_name(&sink->sources, source->name)) {
			log_debug("Source by that name already exists");
			fibril_mutex_unlock(&hound->list_guard);
			return EEXISTS;
		}
	}
	list_append(&source->link, &hound->sources);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

int hound_add_sink(hound_t *hound, audio_sink_t *sink)
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
		return EEXISTS;
	}
	list_append(&sink->link, &hound->sinks);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

int hound_remove_source(hound_t *hound, audio_source_t *source)
{
	assert(hound);
	if (!source)
		return EINVAL;
	log_verbose("Removing source '%s'.", source->name);
	fibril_mutex_lock(&hound->list_guard);
	if (!list_member(&source->link, &hound->sources)) {
		assert(source->connected_sink);
		hound_disconnect_internal(hound, source->name,
		    source->connected_sink->name);
	}
	list_remove(&source->link);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

int hound_remove_sink(hound_t *hound, audio_sink_t *sink)
{
	assert(hound);
	if (!sink)
		return EINVAL;
	log_verbose("Removing sink '%s'.", sink->name);
	fibril_mutex_lock(&hound->list_guard);

	if (!list_empty(&sink->sources)) {
		// TODO disconnect instead
		fibril_mutex_unlock(&hound->list_guard);
		return EBUSY;
	}
	list_remove(&sink->link);
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

int hound_connect(hound_t *hound, const char* source_name, const char* sink_name)
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
	list_remove(&source->link);
	const int ret = audio_sink_add_source(sink, source);
	if (ret != EOK) {
		log_debug("Failed add source to sink list: %s", str_error(ret));
		list_append(&source->link, &hound->sources);
	}
	fibril_mutex_unlock(&hound->list_guard);
	return EOK;
}

int hound_disconnect(hound_t *hound, const char* source_name, const char* sink_name)
{
	assert(hound);
	fibril_mutex_lock(&hound->list_guard);
	const int ret = hound_disconnect_internal(hound, source_name, sink_name);
	fibril_mutex_unlock(&hound->list_guard);
	return ret;
}

static int hound_disconnect_internal(hound_t *hound, const char* source_name, const char* sink_name)
{
	assert(hound);
	assert(fibril_mutex_is_locked(&hound->list_guard));
	log_verbose("Disconnecting '%s' to '%s'.", source_name, sink_name);

	audio_sink_t *sink =
	    audio_sink_list_instance(list_first(&hound->sinks));
	if (str_cmp(sink_name, "default") != 0)
	    sink = find_sink_by_name(&hound->sinks, sink_name);

	audio_source_t *source =
	    audio_source_list_instance(list_first(&hound->sources));
	if (str_cmp(source_name, "default") != 0)
	    source = sink ? find_source_by_name(&sink->sources, source_name) : NULL;
	if (!source || !sink) {
		log_debug("Source (%p), or sink (%p) not found", source, sink);
		return ENOENT;
	}
	const int ret = audio_sink_remove_source(sink, source);
	if (ret != EOK) {
		log_debug("Failed remove source to sink list: %s", str_error(ret));
	} else {
		list_append(&source->link, &hound->sources);
	}
	return EOK;
}
/**
 * @}
 */
