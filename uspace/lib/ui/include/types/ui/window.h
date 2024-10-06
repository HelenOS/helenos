/*
 * Copyright (c) 2024 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <types/common.h>
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
	/** Place window to the center of the screen */
	ui_wnd_place_center,
	/** Place window accross the entire screen */
	ui_wnd_place_full_screen,
	/** Place window as a popup window adjacent to rectangle */
	ui_wnd_place_popup
} ui_wnd_placement_t;

/** Window flags */
typedef enum {
	/** Popup window */
	ui_wndf_popup = 0x1,
	/** Window does not receive focus */
	ui_wndf_nofocus = 0x2,
	/** Topmost window */
	ui_wndf_topmost = 0x4,
	/** Special system window */
	ui_wndf_system = 0x8,
	/** Maximized windows should avoid this window */
	ui_wndf_avoid = 0x10
} ui_wnd_flags_t;

/** Window parameters */
typedef struct {
	/** Window rectangle */
	gfx_rect_t rect;
	/** Minimum size to which window can be resized */
	gfx_coord2_t min_size;
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
	/** Input device associated with the window's seat */
	sysarg_t idev_id;
} ui_wnd_params_t;

/** Window callbacks */
typedef struct ui_window_cb {
	void (*sysmenu)(ui_window_t *, void *, sysarg_t);
	void (*minimize)(ui_window_t *, void *);
	void (*maximize)(ui_window_t *, void *);
	void (*unmaximize)(ui_window_t *, void *);
	void (*resize)(ui_window_t *, void *);
	void (*close)(ui_window_t *, void *);
	void (*focus)(ui_window_t *, void *, unsigned);
	void (*kbd)(ui_window_t *, void *, kbd_event_t *);
	errno_t (*paint)(ui_window_t *, void *);
	void (*pos)(ui_window_t *, void *, pos_event_t *);
	void (*unfocus)(ui_window_t *, void *, unsigned);
} ui_window_cb_t;

#endif

/** @}
 */
