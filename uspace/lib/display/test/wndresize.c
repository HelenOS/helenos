/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
