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
#include <pcm/sample_format.h>

#include "dsp.h"
#include "sb16.h"

static inline sb_dsp_t *fun_to_dsp(ddf_fun_t *fun)
{
	sb16_t *sb = (sb16_t *)ddf_dev_data_get(ddf_fun_get_dev(fun));
	return &sb->dsp;
}

static errno_t sb_get_info_str(ddf_fun_t *fun, const char** name)
{
	if (name)
		*name = "SB 16 DSP";
	return EOK;
}

static unsigned sb_query_cap(ddf_fun_t *fun, audio_cap_t cap)
{
	return sb_dsp_query_cap(fun_to_dsp(fun), cap);
}

static errno_t sb_test_format(ddf_fun_t *fun, unsigned *channels, unsigned *rate,
    pcm_sample_format_t *format)
{
	return sb_dsp_test_format(fun_to_dsp(fun), channels, rate, format);
}
static errno_t sb_get_buffer(ddf_fun_t *fun, void **buffer, size_t *size)
{
	return sb_dsp_get_buffer(fun_to_dsp(fun), buffer, size);
}

static errno_t sb_get_buffer_position(ddf_fun_t *fun, size_t *size)
{
	return sb_dsp_get_buffer_position(fun_to_dsp(fun), size);
}

static errno_t sb_set_event_session(ddf_fun_t *fun, async_sess_t *sess)
{
	return sb_dsp_set_event_session(fun_to_dsp(fun), sess);
}

static async_sess_t * sb_get_event_session(ddf_fun_t *fun)
{
	return sb_dsp_get_event_session(fun_to_dsp(fun));
}

static errno_t sb_release_buffer(ddf_fun_t *fun)
{
	return sb_dsp_release_buffer(fun_to_dsp(fun));
}

static errno_t sb_start_playback(ddf_fun_t *fun, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	return sb_dsp_start_playback(
	    fun_to_dsp(fun), frames, channels, sample_rate, format);
}

static errno_t sb_stop_playback(ddf_fun_t *fun, bool immediate)
{
	return sb_dsp_stop_playback(fun_to_dsp(fun), immediate);
}

static errno_t sb_start_capture(ddf_fun_t *fun, unsigned frames,
    unsigned channels, unsigned sample_rate, pcm_sample_format_t format)
{
	return sb_dsp_start_capture(
	    fun_to_dsp(fun), frames, channels, sample_rate, format);
}

static errno_t sb_stop_capture(ddf_fun_t *fun, bool immediate)
{
	return sb_dsp_stop_capture(fun_to_dsp(fun), immediate);
}

audio_pcm_iface_t sb_pcm_iface = {
	.get_info_str = sb_get_info_str,
	.test_format = sb_test_format,
	.query_cap = sb_query_cap,

	.get_buffer = sb_get_buffer,
	.release_buffer = sb_release_buffer,
	.set_event_session = sb_set_event_session,
	.get_event_session = sb_get_event_session,
	.get_buffer_pos = sb_get_buffer_position,

	.start_playback = sb_start_playback,
	.stop_playback = sb_stop_playback,

	.start_capture = sb_start_capture,
	.stop_capture = sb_stop_capture,
};
/**
 * @}
 */
