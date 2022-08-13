/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_PRIVATE_PARAMS_H_
#define _LIBDISPLAY_PRIVATE_PARAMS_H_

#include <gfx/coord.h>

/** Window resize arguments. */
typedef struct {
	/** Offset */
	gfx_coord2_t offs;
	/** New bounding rectangle */
	gfx_rect_t nrect;
} display_wnd_resize_t;

#endif

/** @}
 */
