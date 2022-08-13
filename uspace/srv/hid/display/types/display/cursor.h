/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server cursor type
 */

#ifndef TYPES_DISPLAY_CURSOR_H
#define TYPES_DISPLAY_CURSOR_H

#include <adt/list.h>
#include <gfx/bitmap.h>
#include <gfx/coord.h>

/** Bitmap in display server window GC */
typedef struct ds_cursor {
	/** Containing display */
	struct ds_display *display;
	/** Link to list of all cursors in display */
	link_t ldisplay;
	/** Bitmap in the display device */
	gfx_bitmap_t *bitmap;
	/** Rectangle represented in the image */
	gfx_rect_t rect;
	/** Cursor image */
	const uint8_t *image;
} ds_cursor_t;

#endif

/** @}
 */
