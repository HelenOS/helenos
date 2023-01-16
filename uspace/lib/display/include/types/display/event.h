/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** Display window focus event */
typedef struct {
	/** New number of foci */
	unsigned nfocus;
} display_wnd_focus_ev_t;

/** Display window resize event */
typedef struct {
	gfx_rect_t rect;
} display_wnd_resize_ev_t;

/** Display window unfocus event */
typedef struct {
	/** Number of remaining foci */
	unsigned nfocus;
} display_wnd_unfocus_ev_t;

/** Display window event */
typedef struct {
	/** Event type */
	display_wnd_ev_type_t etype;
	union {
		/** Focus event data */
		display_wnd_focus_ev_t focus;
		/** Keyboard event data */
		kbd_event_t kbd;
		/** Position event data */
		pos_event_t pos;
		/** Resize event data */
		display_wnd_resize_ev_t resize;
		/** Unfocus event data */
		display_wnd_unfocus_ev_t unfocus;
	} ev;
} display_wnd_ev_t;

#endif

/** @}
 */
