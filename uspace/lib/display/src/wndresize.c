/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <display/wndresize.h>
#include <stdbool.h>

/** Get stock cursor to use for the specified window resize type.
 *
 * The resize type must be valid, otherwise behavior is undefined.
 * Resize type can be validated using @c display_wndrsz_valid().
 *
 * @param rsztype Resize type
 * @return Cursor to use for this resize type
 */
display_stock_cursor_t display_cursor_from_wrsz(display_wnd_rsztype_t rsztype)
{
	switch (rsztype) {
	case display_wr_top:
	case display_wr_bottom:
		return dcurs_size_ud;

	case display_wr_left:
	case display_wr_right:
		return dcurs_size_lr;

	case display_wr_top_left:
	case display_wr_bottom_right:
		return dcurs_size_uldr;

	case display_wr_top_right:
	case display_wr_bottom_left:
		return dcurs_size_urdl;

	default:
		assert(false);
		return dcurs_arrow;
	}
}

/** Determine if resize type is valid.
 *
 * Determine if the resize type has one of the valid values. This function
 * should be used when the resize type comes from an untrusted source
 * (such as from the client).
 *
 * @param rsztype Resize type
 * @return @c true iff the resize type is valid
 */
bool display_wndrsz_valid(display_wnd_rsztype_t rsztype)
{
	bool valid;

	valid = false;

	switch (rsztype) {
	case display_wr_top:
	case display_wr_bottom:
	case display_wr_left:
	case display_wr_right:
	case display_wr_top_left:
	case display_wr_bottom_right:
	case display_wr_top_right:
	case display_wr_bottom_left:
		valid = true;
		break;
	}

	return valid;
}

/** @}
 */
