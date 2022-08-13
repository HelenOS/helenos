/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_TYPES_DISPLAY_CURSOR_H_
#define _LIBDISPLAY_TYPES_DISPLAY_CURSOR_H_

/** Stock cursor types */
typedef enum {
	/** Standard arrow */
	dcurs_arrow,
	/** Double arrow pointing up and down */
	dcurs_size_ud,
	/** Double arrow pointing left and right */
	dcurs_size_lr,
	/** Double arrow pointing up-left and down-right */
	dcurs_size_uldr,
	/** Double arrow pointing up-right nad down-left */
	dcurs_size_urdl,
	/** I-beam (suggests editable text) */
	dcurs_ibeam
} display_stock_cursor_t;

enum {
	/** Number of stock cursor types */
	dcurs_limit = dcurs_ibeam + 1
};

#endif

/** @}
 */
