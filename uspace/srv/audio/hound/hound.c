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
#include "audio_device.h"
#include "audio_sink.h"
#include "audio_source.h"
#include "log.h"
#include "errno.h"
#include "str_error.h"

int hound_init(hound_t *hound)
{
	assert(hound);
	list_initialize(&hound->devices);
	list_initialize(&hound->available_sources);
	list_initialize(&hound->sinks);
	list_initialize(&hound->clients);
	return EOK;
}

int hound_add_device(hound_t *hound, service_id_t id, const char* name)
{
	log_verbose("Adding device \"%s\", service: %zu", name, id);

	assert(hound);
	if (!name || !id) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}

	list_foreach(hound->devices, it) {
		audio_device_t *dev = list_audio_device_instance(it);
		if (dev->id == id) {
			log_debug("Device with id %zu is already present", id);
			return EEXISTS;
		}
	}

	audio_device_t *dev = malloc(sizeof(audio_device_t));
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
	log_info("Added new device: '%s'", name);

	audio_source_t *source = audio_device_get_source(dev);
	if (source) {
		log_verbose("Added source: '%s'.", source->name);
		list_append(&source->link, &hound->available_sources);
	}

	audio_sink_t *sink = audio_device_get_sink(dev);
	if (sink) {
		log_verbose("Added sink: '%s'.", sink->name);
		list_append(&sink->link, &hound->sinks);
	}

	if (!source && !sink)
		log_warning("Neither sink nor source on device '%s'.", name);

	return ret;
}

/**
 * @}
 */
