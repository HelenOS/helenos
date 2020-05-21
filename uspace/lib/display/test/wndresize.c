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

#include <display/wndresize.h>
#include <errno.h>
#include <limits.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(wndresize);

/** display_cursor_from_wrsz() correctly translates resize type to cursor */
PCUT_TEST(cursor_from_wrsz)
{
	PCUT_ASSERT_INT_EQUALS(dcurs_size_ud,
	    display_cursor_from_wrsz(display_wr_top));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_ud,
	    display_cursor_from_wrsz(display_wr_bottom));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_lr,
	    display_cursor_from_wrsz(display_wr_left));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_lr,
	    display_cursor_from_wrsz(display_wr_right));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_uldr,
	    display_cursor_from_wrsz(display_wr_top_left));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_uldr,
	    display_cursor_from_wrsz(display_wr_bottom_right));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_urdl,
	    display_cursor_from_wrsz(display_wr_top_right));
	PCUT_ASSERT_INT_EQUALS(dcurs_size_urdl,
	    display_cursor_from_wrsz(display_wr_bottom_left));
}

/** display_wndrsz_valid() correctly verifies window resize type */
PCUT_TEST(wndrsz_valid)
{
	PCUT_ASSERT_FALSE(display_wndrsz_valid(INT_MIN));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(display_wr_top - 1));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_top));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_left));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_bottom));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_right));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_top_left));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_top_right));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_bottom_left));
	PCUT_ASSERT_TRUE(display_wndrsz_valid(display_wr_bottom_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(display_wr_bottom_right + 1));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_top | display_wr_bottom));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_left | display_wr_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_top | display_wr_left | display_wr_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_bottom | display_wr_left | display_wr_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_top | display_wr_bottom | display_wr_left));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_top | display_wr_bottom | display_wr_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(
	    display_wr_top | display_wr_bottom |
	    display_wr_left | display_wr_right));
	PCUT_ASSERT_FALSE(display_wndrsz_valid(INT_MAX));
}

PCUT_EXPORT(wndresize);
