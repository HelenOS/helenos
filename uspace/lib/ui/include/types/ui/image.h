/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Image
 */

#ifndef _UI_TYPES_IMAGE_H
#define _UI_TYPES_IMAGE_H

struct ui_image;
typedef struct ui_image ui_image_t;

/** UI image flags */
typedef enum {
	/** Draw a frame around the image */
	ui_imgf_frame = 0x1
} ui_image_flags_t;

#endif

/** @}
 */
