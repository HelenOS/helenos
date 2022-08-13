/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Window
 */

#ifndef _UI_TYPES_WINDOW_H
#define _UI_TYPES_WINDOW_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/wdecor.h>

struct ui_window;
typedef struct ui_window ui_window_t;

/** Window placement hint */
typedef enum {
	/** Use default (automatic) placement */
	ui_wnd_place_default = 0,
	/** Place window to the top-left corner of the screen */
	ui_wnd_place_top_left,
	/** Place window to the top-right corner of the screen */
	ui_wnd_place_top_right,
	/** Place window to the bottom-left corner of the screen */
	ui_wnd_place_bottom_left,
	/** Place window to the bottom-right corner of the screen */
	ui_wnd_place_bottom_right,
	/** Place window accross the entire screen */
	ui_wnd_place_full_screen,
	/** Place window as a popup window adjacent to rectangle */
	ui_wnd_place_popup
} ui_wnd_placement_t;

/** Window flags */
typedef enum {
	/** Popup window */
	ui_wndf_popup = 0x1
} ui_wnd_flags_t;

/** Window parameters */
typedef struct {
	/** Window rectangle */
	gfx_rect_t rect;
	/** Window caption */
	const char *caption;
	/** Window decoration style */
	ui_wdecor_style_t style;
	/** Window placement */
	ui_wnd_placement_t placement;
	/** Window flags */
	ui_wnd_flags_t flags;
	/** Parent rectangle for popup windows */
	gfx_rect_t prect;
} ui_wnd_params_t;

/** Window callbacks */
typedef struct ui_window_cb {
	void (*maximize)(ui_window_t *, void *);
	void (*unmaximize)(ui_window_t *, void *);
	void (*close)(ui_window_t *, void *);
	void (*focus)(ui_window_t *, void *);
	void (*kbd)(ui_window_t *, void *, kbd_event_t *);
	errno_t (*paint)(ui_window_t *, void *);
	void (*pos)(ui_window_t *, void *, pos_event_t *);
	void (*unfocus)(ui_window_t *, void *);
} ui_window_cb_t;

#endif

/** @}
 */
