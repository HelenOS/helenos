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
#include <inttypes.h>
#include <loc.h>
#include <str.h>
#include <str_error.h>
#include <as.h>


#include "audio_device.h"
#include "log.h"

/* hardwired to provide ~21ms per fragment */
#define BUFFER_PARTS   16

static errno_t device_sink_connection_callback(audio_sink_t *sink, bool new);
static errno_t device_source_connection_callback(audio_source_t *source, bool new);
static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static errno_t device_check_format(audio_sink_t* sink);
static errno_t get_buffer(audio_device_t *dev);
static errno_t release_buffer(audio_device_t *dev);
static void advance_buffer(audio_device_t *dev, size_t size);
static inline bool is_running(audio_device_t *dev)
{
	assert(dev);
	/* we release buffer on stop so this should be enough */
	return dev->buffer.base != NULL;
}

/**
 * Initialize audio device structure.
 * @param dev The structure to initialize.
 * @param id Location service id of the device driver.
 * @param name Name of the device.
 * @return Error code.
 */
errno_t audio_device_init(audio_device_t *dev, service_id_t id, const char *name)
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
	    device_check_format, NULL, &AUDIO_FORMAT_ANY);
	audio_source_init(&dev->source, name, dev,
	    device_source_connection_callback, NULL, &AUDIO_FORMAT_ANY);

	/* Init buffer members */
	dev->buffer.base = NULL;
	dev->buffer.position = NULL;
	dev->buffer.size = 0;
	dev->buffer.fragment_size = 0;

	log_verbose("Initialized device (%p) '%s' with id %" PRIun ".",
	    dev, dev->name, dev->id);

	return EOK;
}

/**
 * Restore resource cplaimed during initialization.
 * @param dev The device to release.
 *
 * NOT IMPLEMENTED
 */
void audio_device_fini(audio_device_t *dev)
{
	//TODO implement;
}

/**
 * Get device provided audio source.
 * @param dev Th device.
 * @return pointer to aa audio source structure, NULL if the device is not
 *         capable of capturing audio.
 */
audio_source_t * audio_device_get_source(audio_device_t *dev)
{
	assert(dev);
	sysarg_t val;
	errno_t rc = audio_pcm_query_cap(dev->sess, AUDIO_CAP_CAPTURE, &val);
	if (rc == EOK && val)
		return &dev->source;
	return NULL;
}

/**
 * Get device provided audio sink.
 * @param dev Th device.
 * @return pointer to aa audio source structure, NULL if the device is not
 *         capable of audio playback.
 */
audio_sink_t * audio_device_get_sink(audio_device_t *dev)
{
	assert(dev);
	sysarg_t val;
	errno_t rc = audio_pcm_query_cap(dev->sess, AUDIO_CAP_PLAYBACK, &val);
	if (rc == EOK && val)
		return &dev->sink;
	return NULL;
}

/**
 * Handle connection addition and removal.
 * @param sink audio sink that is connected or disconnected.
 * @param new True of a connection was added, false otherwise.
 * @return Error code.
 *
 * Starts playback on first connection. Stops playback when there are no
 * connections.
 */
static errno_t device_sink_connection_callback(audio_sink_t* sink, bool new)
{
	assert(sink);
	audio_device_t *dev = sink->private_data;
	if (new && list_count(&sink->connections) == 1) {
		log_verbose("First connection on device sink '%s'", sink->name);

		errno_t ret = get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}
		audio_pcm_register_event_callback(dev->sess,
		    device_event_callback, dev);

		/* Fill the buffer first. Fill the first two fragments,
		 * so that we stay one fragment ahead */
		pcm_format_silence(dev->buffer.base, dev->buffer.size,
		    &dev->sink.format);
		//TODO add underrun detection.
		const size_t size = dev->buffer.fragment_size * 2;
		/* We never cross the end of the buffer here */
		audio_sink_mix_inputs(&dev->sink, dev->buffer.position, size);
		advance_buffer(dev, size);

		const unsigned frames = dev->buffer.fragment_size /
		    pcm_format_frame_size(&dev->sink.format);
		log_verbose("Fragment frame count %u", frames);
		ret = audio_pcm_start_playback_fragment(dev->sess, frames,
		    dev->sink.format.channels, dev->sink.format.sampling_rate,
		    dev->sink.format.sample_format);
		if (ret != EOK) {
			log_error("Failed to start playback: %s",
			    str_error(ret));
			release_buffer(dev);
			return ret;
		}
	}
	if (list_count(&sink->connections) == 0) {
		assert(!new);
		log_verbose("Removed last connection on device sink '%s'",
		    sink->name);
		errno_t ret = audio_pcm_stop_playback(dev->sess);
		if (ret != EOK) {
			log_error("Failed to stop playback: %s",
			    str_error(ret));
			return ret;
		}
	}
	return EOK;
}

/**
 * Handle connection addition and removal.
 * @param source audio source that is connected or disconnected.
 * @param new True of a connection was added, false otherwise.
 * @return Error code.
 *
 * Starts capture on first connection. Stops capture when there are no
 * connections.
 */
static errno_t device_source_connection_callback(audio_source_t *source, bool new)
{
	assert(source);
	audio_device_t *dev = source->private_data;
	if (new && list_count(&source->connections) == 1) {
		errno_t ret = get_buffer(dev);
		if (ret != EOK) {
			log_error("Failed to get device buffer: %s",
			    str_error(ret));
			return ret;
		}

		//TODO set and test format

		const unsigned frames = dev->buffer.fragment_size /
		    pcm_format_frame_size(&dev->sink.format);
		ret = audio_pcm_start_capture_fragment(dev->sess, frames,
		    dev->source.format.channels,
		    dev->source.format.sampling_rate,
		    dev->source.format.sample_format);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			release_buffer(dev);
			return ret;
		}
	}
	if (list_count(&source->connections) == 0) { /* Disconnected */
		assert(!new);
		errno_t ret = audio_pcm_stop_capture_immediate(dev->sess);
		if (ret != EOK) {
			log_error("Failed to start recording: %s",
			    str_error(ret));
			return ret;
		}
	}

	return EOK;
}

/**
 * Audio device event handler.
 * @param iid initial call id.
 * @param icall initial call structure.
 * @param arg (unused)
 */
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
			struct timeval time1;
			getuptime(&time1);
			//TODO add underrun detection.
			/* We never cross the end of the buffer here */
			audio_sink_mix_inputs(&dev->sink, dev->buffer.position,
			    dev->buffer.fragment_size);
			advance_buffer(dev, dev->buffer.fragment_size);
			struct timeval time2;
			getuptime(&time2);
			log_verbose("Time to mix sources: %li\n",
			    tv_sub_diff(&time2, &time1));
			break;
		}
		case PCM_EVENT_CAPTURE_TERMINATED: {
			log_verbose("Capture terminated");
			dev->source.format = AUDIO_FORMAT_ANY;
			const errno_t ret = release_buffer(dev);
			if (ret != EOK) {
				log_error("Failed to release buffer: %s",
				    str_error(ret));
			}
			audio_pcm_unregister_event_callback(dev->sess);
			break;
		}
		case PCM_EVENT_PLAYBACK_TERMINATED: {
			log_verbose("Playback Terminated");
			dev->sink.format = AUDIO_FORMAT_ANY;
			const errno_t ret = release_buffer(dev);
			if (ret != EOK) {
				log_error("Failed to release buffer: %s",
				    str_error(ret));
			}
			audio_pcm_unregister_event_callback(dev->sess);
			break;
		}
		case PCM_EVENT_FRAMES_CAPTURED: {
			const errno_t ret = audio_source_push_data(&dev->source,
			    dev->buffer.position, dev->buffer.fragment_size);
			advance_buffer(dev, dev->buffer.fragment_size);
			if (ret != EOK)
				log_warning("Failed to push recorded data");
			break;
		}
		case 0:
			log_info("Device event callback hangup");
			return;
		}

	}
}

/**
 * Test format against hardware limits.
 * @param sink audio playback device.
 * @return Error code.
 */
static errno_t device_check_format(audio_sink_t* sink)
{
	assert(sink);
	audio_device_t *dev = sink->private_data;
	assert(dev);
	/* Check whether we are running */
	if (is_running(dev))
		return EBUSY;
	log_verbose("Checking format on sink %s", sink->name);
	return audio_pcm_test_format(dev->sess, &sink->format.channels,
	    &sink->format.sampling_rate, &sink->format.sample_format);
}

/**
 * Get access to device buffer.
 * @param dev Audio device.
 * @return Error code.
 */
static errno_t get_buffer(audio_device_t *dev)
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

	/* Ask for largest buffer possible */
	size_t preferred_size = 0;

	const errno_t ret = audio_pcm_get_buffer(dev->sess, &dev->buffer.base,
	    &preferred_size);
	if (ret == EOK) {
		dev->buffer.size = preferred_size;
		dev->buffer.fragment_size = dev->buffer.size / BUFFER_PARTS;
		dev->buffer.position = dev->buffer.base;
	}
	return ret;

}

/**
 * Surrender access to device buffer.
 * @param dev Audio device.
 * @return Error code.
 */
static errno_t release_buffer(audio_device_t *dev)
{
	assert(dev);
	assert(dev->buffer.base);

	const errno_t ret = audio_pcm_release_buffer(dev->sess);
	if (ret == EOK) {
		as_area_destroy(dev->buffer.base);
		dev->buffer.base = NULL;
		dev->buffer.size = 0;
		dev->buffer.position = NULL;
	} else {
		log_warning("Failed to release buffer: %s", str_error(ret));
	}
	return ret;
}

/**
 * Move buffer position pointer.
 * @param dev Audio device.
 * @param size number of bytes to move forward
 */
static void advance_buffer(audio_device_t *dev, size_t size)
{
	assert(dev);
	assert(dev->buffer.position >= dev->buffer.base);
	assert(dev->buffer.position < (dev->buffer.base + dev->buffer.size));
	dev->buffer.position += size;
	if (dev->buffer.position == (dev->buffer.base + dev->buffer.size))
		dev->buffer.position = dev->buffer.base;
}
/**
 * @}
 */
