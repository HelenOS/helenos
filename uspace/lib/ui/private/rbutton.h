/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Radio button structure
 *
 */

#ifndef _UI_PRIVATE_RBUTTON_H
#define _UI_PRIVATE_RBUTTON_H

#include <gfx/coord.h>
#include <stdbool.h>

/** Actual structure of radio button group.
 *
 * This is private to libui.
 */
struct ui_rbutton_group {
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_rbutton_group_cb *cb;
	/** Callback argument */
	void *arg;
	/** Selected button */
	struct ui_rbutton *selected;
};

/** Actual structure of radio button.
 *
 * This is private to libui.
 */
struct ui_rbutton {
	/** Base control object */
	struct ui_control *control;
	/** Containing radio button group */
	struct ui_rbutton_group *group;
	/** Callback argument */
	void *arg;
	/** Radio button rectangle */
	gfx_rect_t rect;
	/** Caption */
	const char *caption;
	/** Radio button is currently held down */
	bool held;
	/** Pointer is currently inside */
	bool inside;
};

extern errno_t ui_rbutton_paint_gfx(ui_rbutton_t *);
extern errno_t ui_rbutton_paint_text(ui_rbutton_t *);

#endif

/** @}
 */
