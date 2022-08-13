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
static inline audio_device_t *audio_device_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_device_t, link) : NULL;
}

errno_t audio_device_init(audio_device_t *dev, service_id_t id, const char *name);
void audio_device_fini(audio_device_t *dev);
audio_source_t *audio_device_get_source(audio_device_t *dev);
audio_sink_t *audio_device_get_sink(audio_device_t *dev);
errno_t audio_device_recorded_data(audio_device_t *dev, void **base, size_t *size);
errno_t audio_device_available_buffer(audio_device_t *dev, void **base, size_t *size);

#endif

/**
 * @}
 */
