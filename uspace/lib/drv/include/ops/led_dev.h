/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_OPS_LED_DEV_H_
#define LIBDRV_OPS_LED_DEV_H_

#include <io/pixel.h>
#include "../ddf/driver.h"

typedef struct {
	errno_t (*color_set)(ddf_fun_t *, pixel_t);
} led_dev_ops_t;

#endif

/**
 * @}
 */
