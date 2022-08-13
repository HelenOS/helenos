/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Image structure
 *
 */

#ifndef _UI_PRIVATE_IMAGE_H
#define _UI_PRIVATE_IMAGE_H

#include <gfx/bitmap.h>
#include <gfx/coord.h>

/** Actual structure of image.
 *
 * This is private to libui.
 */
struct ui_image {
	/** Base control object */
	struct ui_control *control;
	/** UI resource */
	struct ui_resource *res;
	/** Image rectangle */
	gfx_rect_t rect;
	/** Flags */
	ui_image_flags_t flags;
	/** Bitmap */
	gfx_bitmap_t *bitmap;
	/** Bitmap rectangle */
	gfx_rect_t brect;
};

#endif

/** @}
 */
