/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @addtogroup usb
 * @{
 */
/** @file
 * @brief Audio mixer control interface.
 */

#ifndef LIBDRV_AUDIO_MIXER_IFACE_H_
#define LIBDRV_AUDIO_MIXER_IFACE_H_

#include <async.h>
#include <stdbool.h>

#include "ddf/driver.h"

errno_t audio_mixer_get_info(async_exch_t *, char **, unsigned *);
errno_t audio_mixer_get_item_info(async_exch_t *, unsigned,
    char **, unsigned *);
errno_t audio_mixer_get_item_level(async_exch_t *, unsigned, unsigned *);
errno_t audio_mixer_set_item_level(async_exch_t *, unsigned, unsigned);

/** Audio mixer communication interface. */
typedef struct {
	errno_t (*get_info)(ddf_fun_t *, const char **, unsigned *);
	errno_t (*get_item_info)(ddf_fun_t *, unsigned, const char **, unsigned *);
	errno_t (*get_item_level)(ddf_fun_t *, unsigned, unsigned *);
	errno_t (*set_item_level)(ddf_fun_t *, unsigned, unsigned);
} audio_mixer_iface_t;

#endif
/**
 * @}
 */
