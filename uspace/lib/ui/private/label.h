/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Label structure
 *
 */

#ifndef _UI_PRIVATE_LABEL_H
#define _UI_PRIVATE_LABEL_H

#include <gfx/coord.h>
#include <gfx/text.h>

/** Actual structure of label.
 *
 * This is private to libui.
 */
struct ui_label {
	/** Base control object */
	struct ui_control *control;
	/** UI resource */
	struct ui_resource *res;
	/** Label rectangle */
	gfx_rect_t rect;
	/** Horizontal alignment */
	gfx_halign_t halign;
	/** Vertical alignment */
	gfx_valign_t valign;
	/** Text */
	char *text;
};

#endif

/** @}
 */
