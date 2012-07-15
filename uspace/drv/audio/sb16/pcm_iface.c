/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * Main routines of Creative Labs SoundBlaster 16 driver
 */

#include <async.h>
#include <errno.h>
#include <audio_pcm_iface.h>
#include <pcm_sample_format.h>

#include "dsp.h"

static int sb_get_info_str(ddf_fun_t *fun, const char** name)
{
	if (name)
		*name = "SB 16 DSP";
	return EOK;
}

static int sb_get_buffer(ddf_fun_t *fun, void **buffer, size_t *size)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_get_buffer(dsp, buffer, size);
}
static int sb_set_event_session(ddf_fun_t *fun, async_sess_t *sess)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_set_event_session(dsp, sess);
}

static int sb_release_buffer(ddf_fun_t *fun)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_release_buffer(dsp);
}

static int sb_start_playback(ddf_fun_t *fun, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_start_playback(
	    dsp, frames, channels, sample_rate, format);
}

static int sb_stop_playback(ddf_fun_t *fun)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_stop_playback(dsp);
}

static int sb_start_record(ddf_fun_t *fun, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_start_record(
	    dsp, frames, channels, sample_rate, format);
}

static int sb_stop_record(ddf_fun_t *fun)
{
	assert(fun);
	assert(fun->driver_data);
	sb_dsp_t *dsp = fun->driver_data;
	return sb_dsp_stop_record(dsp);
}

audio_pcm_iface_t sb_pcm_iface = {
	.get_info_str = sb_get_info_str,

	.get_buffer = sb_get_buffer,
	.release_buffer = sb_release_buffer,
	.set_event_session = sb_set_event_session,

	.start_playback = sb_start_playback,
	.stop_playback = sb_stop_playback,

	.start_record = sb_start_record,
	.stop_record = sb_stop_record
};
/**
 * @}
 */
