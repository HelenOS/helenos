/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator panel types
 */

#ifndef TYPES_PANEL_H
#define TYPES_PANEL_H

#include <gfx/color.h>
#include <gfx/coord.h>
#include <ui/filelist.h>
#include <ui/window.h>
#include <stdint.h>

/** Navigator panel
 *
 * This is a custom UI control.
 */
typedef struct panel {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** Callbacks */
	struct panel_cb *cb;

	/** Callback argument */
	void *cb_arg;

	/** Panel rectangle */
	gfx_rect_t rect;

	/** Panel color */
	gfx_color_t *color;

	/** Active border color */
	gfx_color_t *act_border_color;

	/** @c true iff the panel is active */
	bool active;

	/** File list */
	ui_file_list_t *flist;
} panel_t;

/** Panel callbacks */
typedef struct panel_cb {
	/** Request panel activation */
	void (*activate_req)(void *, panel_t *);
} panel_cb_t;

#endif

/** @}
 */
