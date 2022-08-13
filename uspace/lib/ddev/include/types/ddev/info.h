/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libddev
 * @{
 */
/** @file
 */

#ifndef _LIBDDEV_TYPES_DDEV_INFO_H_
#define _LIBDDEV_TYPES_DDEV_INFO_H_

#include <gfx/coord.h>

/** Display device information */
typedef struct {
	/** Bounding rectangle */
	gfx_rect_t rect;
} ddev_info_t;

#endif

/** @}
 */
