/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2011 Vojtech Horky
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

#include <errno.h>
#include <audio_mixer_iface.h>

#include "mixer.h"

static int sb_get_info(ddf_fun_t *fun, const char** name, unsigned *items)
{
	assert(fun);
	assert(fun->driver_data);
	const sb_mixer_t *mixer = fun->driver_data;
	if (name)
		*name = sb_mixer_type_str(mixer->type);
	if (items)
		*items = sb_mixer_get_control_item_count(mixer);

	return EOK;
}
/*----------------------------------------------------------------------------*/
static int sb_get_item_info(ddf_fun_t *fun, unsigned item, const char** name,
    unsigned *channels)
{
	assert(fun);
	assert(fun->driver_data);
	const sb_mixer_t *mixer = fun->driver_data;
	return
	    sb_mixer_get_control_item_info(mixer, item, name, channels);
}
/*----------------------------------------------------------------------------*/
static int sb_get_channel_info(ddf_fun_t *fun, unsigned item, unsigned channel,
    const char** name, unsigned *levels)
{
	assert(fun);
	assert(fun->driver_data);
	const sb_mixer_t *mixer = fun->driver_data;
	return sb_mixer_get_channel_info(mixer, item, channel, name, levels);
}
/*----------------------------------------------------------------------------*/
static int sb_channel_mute_set(ddf_fun_t *fun, unsigned item, unsigned channel,
    bool mute)
{
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
static int sb_channel_mute_get(ddf_fun_t *fun, unsigned item, unsigned channel,
    bool *mute)
{
	*mute = false;
	return EOK;
}
/*----------------------------------------------------------------------------*/
static int sb_channel_volume_set(ddf_fun_t *fun, unsigned item, unsigned channel,
    unsigned volume)
{
	assert(fun);
	assert(fun->driver_data);
	const sb_mixer_t *mixer = fun->driver_data;
	return sb_mixer_set_volume_level(mixer, item, channel, volume);
}
/*----------------------------------------------------------------------------*/
static int sb_channel_volume_get(ddf_fun_t *fun, unsigned item, unsigned channel,
    unsigned *level, unsigned *max)
{
	assert(fun);
	assert(fun->driver_data);
	const sb_mixer_t *mixer = fun->driver_data;
	unsigned levels;
	const int ret =
	    sb_mixer_get_channel_info(mixer, item, channel, NULL, &levels);
	if (ret == EOK && max)
		*max = --levels;
	if (ret == EOK && level)
		*level = sb_mixer_get_volume_level(mixer, item, channel);

	return ret;
}

audio_mixer_iface_t sb_mixer_iface = {
	.get_info = sb_get_info,
	.get_item_info = sb_get_item_info,
	.get_channel_info = sb_get_channel_info,

	.channel_mute_set = sb_channel_mute_set,
	.channel_mute_get = sb_channel_mute_get,

	.channel_volume_set = sb_channel_volume_set,
	.channel_volume_get = sb_channel_volume_get,

};
/**
 * @}
 */
