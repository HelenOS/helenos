/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server cursor image type
 */

#ifndef TYPES_DISPLAY_CURSIMG_H
#define TYPES_DISPLAY_CURSIMG_H

#include <gfx/coord.h>
#include <stdint.h>

/** Bitmap in display server window GC */
typedef struct {
	gfx_rect_t rect;
	/** Image, 0 = background, 1 = black, 2 = white */
	const uint8_t *image;
} ds_cursimg_t;

#endif

/** @}
 */
