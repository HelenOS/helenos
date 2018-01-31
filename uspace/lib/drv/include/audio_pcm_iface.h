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
/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief Audio PCM buffer interface.
 */

#ifndef LIBDRV_AUDIO_PCM_IFACE_H_
#define LIBDRV_AUDIO_PCM_IFACE_H_

#include <async.h>
#include <stdbool.h>
#include <loc.h>
#include <pcm/sample_format.h>

#include "ddf/driver.h"

typedef enum {
	/** Device is capable of audio capture */
	AUDIO_CAP_CAPTURE,
	/** Device is capable of audio playback */
	AUDIO_CAP_PLAYBACK,
	/** Maximum size of device buffer */
	AUDIO_CAP_MAX_BUFFER,
	/** Device is capable of providing accurate buffer position info */
	AUDIO_CAP_BUFFER_POS,
	/** Device is capable of event based playback or capture */
	AUDIO_CAP_INTERRUPT,
	/** Minimal size of playback/record fragment */
	AUDIO_CAP_INTERRUPT_MIN_FRAMES,
	/** Maximum size of playback/record fragment */
	AUDIO_CAP_INTERRUPT_MAX_FRAMES,
} audio_cap_t;

typedef enum {
	PCM_EVENT_PLAYBACK_STARTED = IPC_FIRST_USER_METHOD,
	PCM_EVENT_CAPTURE_STARTED,
	PCM_EVENT_FRAMES_PLAYED,
	PCM_EVENT_FRAMES_CAPTURED,
	PCM_EVENT_PLAYBACK_TERMINATED,
	PCM_EVENT_CAPTURE_TERMINATED,
} pcm_event_t;

const char *audio_pcm_cap_str(audio_cap_t);
const char *audio_pcm_event_str(pcm_event_t);

typedef async_sess_t audio_pcm_sess_t;

audio_pcm_sess_t *audio_pcm_open(const char *);
audio_pcm_sess_t *audio_pcm_open_default(void);
audio_pcm_sess_t *audio_pcm_open_service(service_id_t service);
void audio_pcm_close(audio_pcm_sess_t *);

errno_t audio_pcm_get_info_str(audio_pcm_sess_t *, char **);
errno_t audio_pcm_test_format(audio_pcm_sess_t *, unsigned *, unsigned *,
    pcm_sample_format_t *);
errno_t audio_pcm_query_cap(audio_pcm_sess_t *, audio_cap_t, sysarg_t *);
errno_t audio_pcm_register_event_callback(audio_pcm_sess_t *,
    async_port_handler_t, void *);
errno_t audio_pcm_unregister_event_callback(audio_pcm_sess_t *);

errno_t audio_pcm_get_buffer(audio_pcm_sess_t *, void **, size_t *);
errno_t audio_pcm_get_buffer_pos(audio_pcm_sess_t *, size_t *);
errno_t audio_pcm_release_buffer(audio_pcm_sess_t *);

errno_t audio_pcm_start_playback_fragment(audio_pcm_sess_t *, unsigned,
    unsigned, unsigned, pcm_sample_format_t);
errno_t audio_pcm_last_playback_fragment(audio_pcm_sess_t *);

errno_t audio_pcm_start_playback(audio_pcm_sess_t *,
    unsigned, unsigned, pcm_sample_format_t);
errno_t audio_pcm_stop_playback_immediate(audio_pcm_sess_t *);
errno_t audio_pcm_stop_playback(audio_pcm_sess_t *);

errno_t audio_pcm_start_capture_fragment(audio_pcm_sess_t *, unsigned,
    unsigned, unsigned, pcm_sample_format_t);
errno_t audio_pcm_last_capture_fragment(audio_pcm_sess_t *);

errno_t audio_pcm_start_capture(audio_pcm_sess_t *,
    unsigned, unsigned, pcm_sample_format_t);
errno_t audio_pcm_stop_capture_immediate(audio_pcm_sess_t *);
errno_t audio_pcm_stop_capture(audio_pcm_sess_t *);

/** Audio pcm communication interface. */
typedef struct {
	errno_t (*get_info_str)(ddf_fun_t *, const char **);
	errno_t (*test_format)(ddf_fun_t *, unsigned *, unsigned *,
	    pcm_sample_format_t *);
	unsigned (*query_cap)(ddf_fun_t *, audio_cap_t);
	errno_t (*get_buffer_pos)(ddf_fun_t *, size_t *);
	errno_t (*get_buffer)(ddf_fun_t *, void **, size_t *);
	errno_t (*release_buffer)(ddf_fun_t *);
	errno_t (*set_event_session)(ddf_fun_t *, async_sess_t *);
	async_sess_t *(*get_event_session)(ddf_fun_t *);
	errno_t (*start_playback)(ddf_fun_t *, unsigned,
	    unsigned, unsigned, pcm_sample_format_t);
	errno_t (*stop_playback)(ddf_fun_t *, bool);
	errno_t (*start_capture)(ddf_fun_t *, unsigned,
	    unsigned, unsigned, pcm_sample_format_t);
	errno_t (*stop_capture)(ddf_fun_t *, bool);
} audio_pcm_iface_t;

#endif
/**
 * @}
 */
