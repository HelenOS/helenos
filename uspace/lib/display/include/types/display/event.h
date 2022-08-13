/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_EVENT_H_
#define _LIBDISPLAY_TYPES_DISPLAY_EVENT_H_

#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>

/** Display window event type */
typedef enum {
	/** Request to close window */
	wev_close,
	/** Window gained focus */
	wev_focus,
	/** Keyboard event */
	wev_kbd,
	/** Position event */
	wev_pos,
	/** Resize event */
	wev_resize,
	/** Window lost focus */
	wev_unfocus
} display_wnd_ev_type_t;

/** Display window resize event */
typedef struct {
	gfx_rect_t rect;
} display_wnd_resize_ev_t;

/** Display window event */
typedef struct {
	/** Event type */
	display_wnd_ev_type_t etype;
	union {
		/** Keyboard event data */
		kbd_event_t kbd;
		/** Position event data */
		pos_event_t pos;
		/** Resize event data */
		display_wnd_resize_ev_t resize;
	} ev;
} display_wnd_ev_t;

#endif

/** @}
 */
