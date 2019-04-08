/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <gfx/color.h>
#include <gfx/render.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(render);

PCUT_TEST(set_color)
{
	errno_t rc;
	gfx_color_t *color;
	gfx_context_t *gc = NULL;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
}

PCUT_TEST(fill_rect)
{
	errno_t rc;
	gfx_color_t *color;
	gfx_rect_t rect;
	gfx_context_t *gc = NULL;

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
}

PCUT_EXPORT(render);
