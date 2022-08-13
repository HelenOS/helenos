/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * Main routines of Creative Labs SoundBlaster 16 driver
 */

#include <ddf/driver.h>
#include <errno.h>
#include <audio_mixer_iface.h>

#include "mixer.h"
#include "sb16.h"

static sb_mixer_t *fun_to_mixer(ddf_fun_t *fun)
{
	sb16_t *sb = (sb16_t *)ddf_dev_data_get(ddf_fun_get_dev(fun));
	return &sb->mixer;
}

static errno_t sb_get_info(ddf_fun_t *fun, const char **name, unsigned *items)
{
	sb_mixer_t *mixer = fun_to_mixer(fun);

	if (name)
		*name = sb_mixer_type_str(mixer->type);
	if (items)
		*items = sb_mixer_get_control_item_count(mixer);

	return EOK;
}

static errno_t sb_get_item_info(ddf_fun_t *fun, unsigned item, const char **name,
    unsigned *max_level)
{
	sb_mixer_t *mixer = fun_to_mixer(fun);
	return sb_mixer_get_control_item_info(mixer, item, name, max_level);
}

static errno_t sb_set_item_level(ddf_fun_t *fun, unsigned item, unsigned value)
{
	sb_mixer_t *mixer = fun_to_mixer(fun);
	return sb_mixer_set_control_item_value(mixer, item, value);
}

static errno_t sb_get_item_level(ddf_fun_t *fun, unsigned item, unsigned *value)
{
	sb_mixer_t *mixer = fun_to_mixer(fun);
	return sb_mixer_get_control_item_value(mixer, item, value);
}

audio_mixer_iface_t sb_mixer_iface = {
	.get_info = sb_get_info,
	.get_item_info = sb_get_item_info,

	.get_item_level = sb_get_item_level,
	.set_item_level = sb_set_item_level,
};
/**
 * @}
 */
