/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio PCM interface
 */

#include <async.h>
#include <audio_pcm_iface.h>
#include <ddf/log.h>
#include <errno.h>
#include <pcm/sample_format.h>
#include <stdbool.h>

static int hda_get_info_str(ddf_fun_t *, const char **);
static unsigned hda_query_cap(ddf_fun_t *, audio_cap_t);
static int hda_test_format(ddf_fun_t *, unsigned *, unsigned *,
    pcm_sample_format_t *);
static int hda_get_buffer(ddf_fun_t *, void **, size_t *);
static int hda_get_buffer_position(ddf_fun_t *, size_t *);
static int hda_set_event_session(ddf_fun_t *, async_sess_t *);
static async_sess_t *hda_get_event_session(ddf_fun_t *);
static int hda_release_buffer(ddf_fun_t *);
static int hda_start_playback(ddf_fun_t *, unsigned, unsigned, unsigned,
    pcm_sample_format_t);
static int hda_stop_playback(ddf_fun_t *, bool);
static int hda_start_capture(ddf_fun_t *, unsigned, unsigned, unsigned,
    pcm_sample_format_t);
static int hda_stop_capture(ddf_fun_t *, bool);

audio_pcm_iface_t hda_pcm_iface = {
	.get_info_str = hda_get_info_str,
	.test_format = hda_test_format,
	.query_cap = hda_query_cap,

	.get_buffer = hda_get_buffer,
	.release_buffer = hda_release_buffer,
	.set_event_session = hda_set_event_session,
	.get_event_session = hda_get_event_session,
	.get_buffer_pos = hda_get_buffer_position,

	.start_playback = hda_start_playback,
	.stop_playback = hda_stop_playback,

	.start_capture = hda_start_capture,
	.stop_capture = hda_stop_capture
};

static int hda_get_info_str(ddf_fun_t *fun, const char **name)
{
	ddf_msg(LVL_NOTE, "hda_get_info_str()");
	return ENOTSUP;
}

static unsigned hda_query_cap(ddf_fun_t *fun, audio_cap_t cap)
{
	ddf_msg(LVL_NOTE, "hda_query_cap()");
	return ENOTSUP;
}

static int hda_test_format(ddf_fun_t *fun, unsigned *channels,
    unsigned *rate, pcm_sample_format_t *format)
{
	ddf_msg(LVL_NOTE, "hda_test_format()");
	return ENOTSUP;
}

static int hda_get_buffer(ddf_fun_t *fun, void **buffer, size_t *size)
{
	ddf_msg(LVL_NOTE, "hda_get_buffer()");
	return ENOTSUP;
}

static int hda_get_buffer_position(ddf_fun_t *fun, size_t *pos)
{
	ddf_msg(LVL_NOTE, "hda_get_buffer_position()");
	return ENOTSUP;
}

static int hda_set_event_session(ddf_fun_t *fun, async_sess_t *sess)
{
	ddf_msg(LVL_NOTE, "hda_set_event_session()");
	return ENOTSUP;
}

static async_sess_t *hda_get_event_session(ddf_fun_t *fun)
{
	ddf_msg(LVL_NOTE, "hda_get_event_session()");
	return NULL;
}

static int hda_release_buffer(ddf_fun_t *fun)
{
	ddf_msg(LVL_NOTE, "hda_release_buffer()");
	return ENOTSUP;
}

static int hda_start_playback(ddf_fun_t *fun, unsigned frames,
    unsigned channels, unsigned rate, pcm_sample_format_t format)
{
	ddf_msg(LVL_NOTE, "hda_start_playback()");
	return ENOTSUP;
}

static int hda_stop_playback(ddf_fun_t *fun, bool immediate)
{
	ddf_msg(LVL_NOTE, "hda_stop_playback()");
	return ENOTSUP;
}

static int hda_start_capture(ddf_fun_t *fun, unsigned frames, unsigned channels,
    unsigned rate, pcm_sample_format_t format)
{
	ddf_msg(LVL_NOTE, "hda_start_capture()");
	return ENOTSUP;
}

static int hda_stop_capture(ddf_fun_t *fun, bool immediate)
{
	ddf_msg(LVL_NOTE, "hda_stop_capture()");
	return ENOTSUP;
}

/** @}
 */
