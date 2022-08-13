/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_DEVICE_LED_DEV_H
#define LIBDEVICE_DEVICE_LED_DEV_H

#include <async.h>
#include <io/pixel.h>

typedef enum {
	LED_DEV_COLOR_SET = 0
} led_dev_method_t;

extern errno_t led_dev_color_set(async_sess_t *, pixel_t);

#endif
