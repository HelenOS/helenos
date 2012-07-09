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
#include <bool.h>

#include "ddf/driver.h"

int audio_pcm_get_info_str(async_exch_t *, const char **);
int audio_pcm_get_buffer(async_exch_t *, void **, size_t *, unsigned *,
    async_client_conn_t, void *);
int audio_pcm_release_buffer(async_exch_t *, unsigned);

int audio_pcm_start_playback(async_exch_t *, unsigned, unsigned,
    unsigned, uint16_t, uint8_t, bool);
int audio_pcm_stop_playback(async_exch_t *, unsigned);

int audio_pcm_start_record(async_exch_t *, unsigned, unsigned,
    unsigned, uint16_t, uint8_t, bool);
int audio_pcm_stop_record(async_exch_t *, unsigned);

/** Audio pcm communication interface. */
typedef struct {
	int (*get_info_str)(ddf_fun_t *, const char **);
	int (*get_buffer)(ddf_fun_t *, void **, size_t *, unsigned *);
	int (*release_buffer)(ddf_fun_t *, unsigned);
	int (*set_event_session)(ddf_fun_t *, unsigned, async_sess_t *);
	int (*start_playback)(ddf_fun_t *, unsigned, unsigned,
	    unsigned, unsigned, unsigned, bool);
	int (*stop_playback)(ddf_fun_t *, unsigned);
	int (*start_record)(ddf_fun_t *, unsigned, unsigned,
	    unsigned, unsigned, unsigned, bool);
	int (*stop_record)(ddf_fun_t *, unsigned);
} audio_pcm_iface_t;

#endif
/**
 * @}
 */
