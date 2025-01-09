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

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_WNDPARAMS_H_
#define _LIBDISPLAY_TYPES_DISPLAY_WNDPARAMS_H_

#include <gfx/coord.h>

/** Window flags */
typedef enum {
	/** Popup window (capture events, no focus) */
	wndf_popup = 0x1,
	/** Window does not receive focus */
	wndf_nofocus = 0x2,
	/** Topmost window */
	wndf_topmost = 0x4,
	/** Set specific initial window position */
	wndf_setpos = 0x8,
	/** Window is minimized */
	wndf_minimized = 0x10,
	/** Window is maximized */
	wndf_maximized = 0x20,
	/** Special system window */
	wndf_system = 0x40,
	/** Maximized windows should avoid this window */
	wndf_avoid = 0x80
} display_wnd_flags_t;

/** Parameters for a new window.
 *
 * The window's dimensions are determined by the bounding rectangle,
 * the position of which does not relate to its position on the display,
 * it just determines which range of logical coordinates is used
 * by the window.
 */
typedef struct {
	/** Bounding rectangle */
	gfx_rect_t rect;
	/** Window caption */
	const char *caption;
	/** Minimum size (when being resized) */
	gfx_coord2_t min_size;
	/** Initial position (if flag wndf_setpos is set) */
	gfx_coord2_t pos;
	/** Flags */
	display_wnd_flags_t flags;
	/** Input device ID associated with window's seat (or zero) */
	sysarg_t idev_id;
} display_wnd_params_t;

#endif

/** @}
 */
