/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Check box structure
 *
 */

#ifndef _UI_PRIVATE_CHECKBOX_H
#define _UI_PRIVATE_CHECKBOX_H

#include <gfx/coord.h>
#include <stdbool.h>

/** Actual structure of check box.
 *
 * This is private to libui.
 */
struct ui_checkbox {
	/** Base control object */
	struct ui_control *control;
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_checkbox_cb *cb;
	/** Callback argument */
	void *arg;
	/** Check box rectangle */
	gfx_rect_t rect;
	/** Caption */
	const char *caption;
	/** Check box is checked */
	bool checked;
	/** Check box is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
};

extern errno_t ui_checkbox_paint_gfx(ui_checkbox_t *);
extern errno_t ui_checkbox_paint_text(ui_checkbox_t *);

#endif

/** @}
 */
