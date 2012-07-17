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

#define BUFFER_PARTS 2

static int device_sink_connection_callback(audio_sink_t *sink, bool new);
static int device_source_connection_callback(audio_source_t *source);
static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int device_check_format(audio_sink_t* sink);
static int get_buffer(audio_device_t *dev);
static int release_buffer(audio_device_t *dev);


int audio_device_init(audio_device_t *dev, service_id_t id, const char *name)
{
	assert(dev);
	link_initialize(&dev->link);
	dev->id = id;
	dev->name = str_dup(name);
	dev->sess = audio_pcm_open_service(id);
	if (!dev->sess) {
		log_debug("Failed to connect to device \"%s\"", name);
		return ENOMEM;
	}

	audio_sink_init(&dev->sink, name, dev, device_sink_connection_callback,
	    device_check_format, &AUDIO_FORMAT_ANY);
	audio_source_init(&dev->source, name, dev,
	    device_source_connection_callback, NULL, &AUDIO_FORMAT_ANY);

	/* Init buffer members */
	fibril_mutex_initialize(&dev->buffer.guard);
	fibril_condvar_initialize(&dev->buffer.wc);
	dev->buffer.base = NULL;
	dev->buffer.position = NULL;
	dev->buffer.size = 0;

	log_verbose("Initialized device (%p) '%s' with id %u.",
	    dev, dev->name, dev->id);

	return EOK;
}
void audio_device_fini(audio_device_t *dev)
{
	//TODO implement;
}

static int device_sink_connection_callback(audio_sink_t* sink, bool new)
{
	assert(sink);
	audio_device_t *dev = sink->private_data;
	if (new && list_count(&sink->sources) == 1) {
		log_verbose("First connection on device sink '%s'", sink->name);

		int ret = get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}

		/* Fill the buffer first */
		audio_sink_mix_inputs(&dev->sink,
		    dev->buffer.base, dev->buffer.size);

		const unsigned frames = dev->buffer.size /
		    (BUFFER_PARTS * pcm_format_frame_size(&dev->sink.format));
		ret = audio_pcm_start_playback(dev->sess, frames,
		    dev->sink.format.channels, dev->sink.format.sampling_rate,
		    dev->sink.format.sample_format);
		if (ret != EOK) {
			log_error("Failed to start playback: %s",
			    str_error(ret));
			release_buffer(dev);
			return ret;
		}
	}
	if (list_count(&sink->sources) == 0) {
		assert(!new);
		log_verbose("No connections on device sink '%s'", sink->name);
		int ret = audio_pcm_stop_playback(dev->sess);
		if (ret != EOK) {
			log_error("Failed to start playback: %s",
			    str_error(ret));
			return ret;
		}
		dev->sink.format = AUDIO_FORMAT_ANY;
		ret = release_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to release buffer: %s",
			    str_error(ret));
			return ret;
		}
	}
	return EOK;
}

static int device_source_connection_callback(audio_source_t *source)
{
	assert(source);
	audio_device_t *dev = source->private_data;
	if (source->connected_sink) {
		int ret = get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}
		const unsigned frames = dev->buffer.size /
		    (BUFFER_PARTS * pcm_format_frame_size(&dev->sink.format));
		ret = audio_pcm_start_record(dev->sess, frames,
		    dev->sink.format.channels, dev->sink.format.sampling_rate,
		    dev->sink.format.sample_format);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			release_buffer(dev);
			return ret;
		}
	} else { /* Disconnected */
		int ret = audio_pcm_stop_record(dev->sess);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			return ret;
		}
		source->format = AUDIO_FORMAT_ANY;
		ret = release_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to release buffer: %s",
			    str_error(ret));
			return ret;
		}
	}

	return EOK;
}

static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Answer initial request */
	async_answer_0(iid, EOK);
	audio_device_t *dev = arg;
	assert(dev);
	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		async_answer_0(callid, EOK);
		switch(IPC_GET_IMETHOD(call)) {
		case PCM_EVENT_FRAMES_PLAYED: {
			//TODO add underrun protection.
			if (dev->buffer.position) {
				dev->buffer.position +=
				    (dev->buffer.size / BUFFER_PARTS);
			}
			if ((!dev->buffer.position) ||
			    (dev->buffer.position >=
			        (dev->buffer.base + dev->buffer.size)))
			{
				dev->buffer.position = dev->buffer.base;
			}
			audio_sink_mix_inputs(&dev->sink, dev->buffer.position,
			    dev->buffer.size / BUFFER_PARTS);
			break;
		}
		case PCM_EVENT_PLAYBACK_TERMINATED: {
			log_verbose("Playback terminated!");
			return;
		}
		case PCM_EVENT_FRAMES_RECORDED: {
			//TODO implement
			break;
		}
		case PCM_EVENT_RECORDING_TERMINATED:
			log_verbose("Recording terminated!");
			return;
		}

	}
}
static int device_check_format(audio_sink_t* sink)
{
	assert(sink);
	audio_device_t *dev = sink->private_data;
	assert(dev);
	log_verbose("Checking format on sink %s", sink->name);
	return audio_pcm_test_format(dev->sess, &sink->format.channels,
	    &sink->format.sampling_rate, &sink->format.sample_format);
}

static int get_buffer(audio_device_t *dev)
{
	assert(dev);
	if (!dev->sess) {
		log_debug("No connection to device");
		return EIO;
	}
	if (dev->buffer.base) {
		log_debug("We already have a buffer");
		return EBUSY;
	}

	dev->buffer.size = 0;

	return audio_pcm_get_buffer(dev->sess, &dev->buffer.base,
	    &dev->buffer.size, device_event_callback, dev);
}

static int release_buffer(audio_device_t *dev)
{
	assert(dev);
	assert(dev->buffer.base);

	const int ret = audio_pcm_release_buffer(dev->sess);
	if (ret == EOK) {
		dev->buffer.base = NULL;
		dev->buffer.size = 0;
		dev->buffer.position = NULL;
	} else {
		log_debug("Failed to release buffer: %s", str_error(ret));
	}
	return ret;
}
/**
 * @}
 */
