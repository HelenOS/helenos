/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/** @file
 */

#ifndef _LIBUI_TYPES_CURSOR_H_
#define _LIBUI_TYPES_CURSOR_H_

/** Stock cursor types */
typedef enum {
	/** Standard arrow */
	ui_curs_arrow,
	/** Double arrow pointing up and down */
	ui_curs_size_ud,
	/** Double arrow pointing left and right */
	ui_curs_size_lr,
	/** Double arrow pointing up-left and down-right */
	ui_curs_size_uldr,
	/** Double arrow pointing up-right nad down-left */
	ui_curs_size_urdl,
	/** I-beam (suggests editable text) */
	ui_curs_ibeam
} ui_stock_cursor_t;

enum {
	/** Number of stock cursor types */
	ui_curs_limit = ui_curs_size_urdl + 1
};

#endif

/** @}
 */
