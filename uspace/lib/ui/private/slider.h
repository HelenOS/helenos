/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Slider structure
 *
 */

#ifndef _UI_PRIVATE_SLIDER_H
#define _UI_PRIVATE_SLIDER_H

#include <gfx/coord.h>
#include <stdbool.h>

/** Actual structure of slider.
 *
 * This is private to libui.
 */
struct ui_slider {
	/** Base control object */
	struct ui_control *control;
	/** UI resource */
	struct ui_resource *res;
	/** Callbacks */
	struct ui_slider_cb *cb;
	/** Callback argument */
	void *arg;
	/** Slider rectangle */
	gfx_rect_t rect;
	/** Slider is currently held down */
	bool held;
	/** Position where button was pressed */
	gfx_coord2_t press_pos;
	/** Last slider position */
	gfx_coord_t last_pos;
	/** Slider position */
	gfx_coord_t pos;
};

extern errno_t ui_slider_paint_gfx(ui_slider_t *);
extern errno_t ui_slider_paint_text(ui_slider_t *);

#endif

/** @}
 */
