/*
 * Copyright (c) 2020 Jiri Svoboda
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
