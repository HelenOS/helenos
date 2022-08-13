/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	/** Set specific initial window position */
	wndf_setpos = 0x2,
	/** Window is maximized */
	wndf_maximized = 0x4
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
	/** Minimum size (when being resized) */
	gfx_coord2_t min_size;
	/** Initial position (if flag wndf_setpos is set) */
	gfx_coord2_t pos;
	/** Flags */
	display_wnd_flags_t flags;
} display_wnd_params_t;

#endif

/** @}
 */
