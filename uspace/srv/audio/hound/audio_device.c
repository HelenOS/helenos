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
#include <async.h>
#include <errno.h>
#include <loc.h>
#include <str.h>
#include <str_error.h>

#include "audio_device.h"
#include "log.h"

static int device_sink_connection_callback(void* arg);
static int device_source_connection_callback(void* arg, const audio_format_t *f);
static int audio_device_get_buffer(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}
static int audio_device_release_buffer(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}
static int audio_device_start_playback(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}
static int audio_device_stop_playback(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}
static int audio_device_start_recording(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}
static int audio_device_stop_recording(audio_device_t *dev)
{
	// TODO implement
	return ENOTSUP;
}


int audio_device_init(audio_device_t *dev, service_id_t id, const char *name)
{
	assert(dev);
	link_initialize(&dev->link);
	dev->id = id;
	dev->name = str_dup(name);
	dev->sess = loc_service_connect(EXCHANGE_SERIALIZE, id, 0);
	if (!dev->sess) {
		log_debug("Failed to connect to device \"%s\"", name);
		return ENOMEM;
	}

	audio_sink_init(&dev->sink, name);
	audio_source_init(&dev->source, name);

	audio_sink_set_connected_callback(&dev->sink,
	    device_sink_connection_callback, dev);
	audio_source_set_connected_callback(&dev->source,
	    device_source_connection_callback, dev);

	log_verbose("Initialized device (%p) '%s' with id %u.",
	    dev, dev->name, dev->id);

	return EOK;
}

static int device_sink_connection_callback(void* arg)
{
	audio_device_t *dev = arg;
	if (list_count(&dev->sink.sources) == 1) {
		int ret = audio_device_get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}
		ret = audio_device_start_playback(dev);
		if (ret != EOK) {
			log_error("Failed to start playback: %s",
			    str_error(ret));
			audio_device_release_buffer(dev);
			return ret;
		}
	}
	if (list_count(&dev->sink.sources) == 0) {
		int ret = audio_device_stop_playback(dev);
		if (ret != EOK) {
			log_error("Failed to start playback: %s",
			    str_error(ret));
			return ret;
		}
		dev->sink.format = AUDIO_FORMAT_ANY;
		ret = audio_device_release_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to release buffer: %s",
			    str_error(ret));
			return ret;
		}
	}
	return EOK;
}

static int device_source_connection_callback(void* arg, const audio_format_t *f)
{
	audio_device_t *dev = arg;
	if (f) { /* Connected, f points to sink format */
		int ret = audio_device_get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}
		ret = audio_device_start_recording(dev);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			audio_device_release_buffer(dev);
			return ret;
		}
		dev->source.format = *f;
	} else { /* Disconnected, f is NULL */
		int ret = audio_device_stop_recording(dev);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			return ret;
		}
		dev->sink.format = AUDIO_FORMAT_ANY;
		ret = audio_device_release_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to release buffer: %s",
			    str_error(ret));
			return ret;
		}
	}

	return EOK;
}


/**
 * @}
 */
