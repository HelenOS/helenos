/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_WNDRESIZE_H_
#define _LIBDISPLAY_TYPES_DISPLAY_WNDRESIZE_H_

/** Window resize type */
typedef enum {
	display_wr_top = 0x1,
	display_wr_left = 0x2,
	display_wr_bottom = 0x4,
	display_wr_right = 0x8,

	display_wr_top_left = display_wr_top | display_wr_left,
	display_wr_bottom_left = display_wr_bottom | display_wr_left,
	display_wr_bottom_right = display_wr_bottom | display_wr_right,
	display_wr_top_right = display_wr_top | display_wr_right
} display_wnd_rsztype_t;

#endif

/** @}
 */
