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

#ifndef AUDIO_DEVICE_H_
#define AUDIO_DEVICE_H_

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <errno.h>
#include <ipc/loc.h>
#include <audio_pcm_iface.h>

#include "audio_source.h"
#include "audio_sink.h"

/** Audio device structure. */
typedef struct {
	/** Link in hound's device list */
	link_t link;
	/** Location service id of the audio driver */
	service_id_t id;
	/** IPC session to the device driver. */
	audio_pcm_sess_t *sess;
	/** Device name */
	char *name;
	/** Device buffer */
	struct {
		void *base;
		size_t size;
		void *position;
		size_t fragment_size;
	} buffer;
	/** Capture device abstraction. */
	audio_source_t source;
	/** Playback device abstraction. */
	audio_sink_t sink;
} audio_device_t;

/** Linked list instance helper */
static inline audio_device_t * audio_device_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_device_t, link) : NULL;
};

errno_t audio_device_init(audio_device_t *dev, service_id_t id, const char *name);
void audio_device_fini(audio_device_t *dev);
audio_source_t * audio_device_get_source(audio_device_t *dev);
audio_sink_t * audio_device_get_sink(audio_device_t *dev);
errno_t audio_device_recorded_data(audio_device_t *dev, void **base, size_t *size);
errno_t audio_device_available_buffer(audio_device_t *dev, void **base, size_t *size);

#endif

/**
 * @}
 */

