/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <gfx/color.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(color);

PCUT_TEST(init_rgb_i16)
{
	errno_t rc;
	gfx_color_t *color;

	/* White */
	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
}

PCUT_EXPORT(color);
