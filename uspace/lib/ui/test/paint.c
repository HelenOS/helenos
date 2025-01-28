/*
 * Copyright (c) 2023 Jiri Svoboda
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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/paint.h>
#include <ui/resource.h>
#include "../private/testgc.h"

PCUT_INIT;

PCUT_TEST_SUITE(paint);

/** Test box characters */
static ui_box_chars_t test_box_chars = {
	{
		{ "A", "B", "C" },
		{ "D", " ", "E" },
		{ "F", "G", "H" }
	}
};

/** Paint bevel */
PCUT_TEST(bevel)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;
	gfx_color_t *color1;
	gfx_color_t *color2;
	gfx_rect_t inside;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_color_new_rgb_i16(4, 5, 6, &color2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint bevel with NULL 'inside' output parameter */
	rc = ui_paint_bevel(gc, &rect, color1, color2, 2, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Paint bevel with valid 'inside' output parameter */
	rc = ui_paint_bevel(gc, &rect, color1, color2, 2, &inside);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color2);
	gfx_color_delete(color1);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Get bevel inside */
PCUT_TEST(get_bevel_inside)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;
	gfx_rect_t inside;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	ui_paint_get_bevel_inside(gc, &rect, 2, &inside);
	PCUT_ASSERT_INT_EQUALS(12, inside.p0.x);
	PCUT_ASSERT_INT_EQUALS(22, inside.p0.y);
	PCUT_ASSERT_INT_EQUALS(28, inside.p1.x);
	PCUT_ASSERT_INT_EQUALS(38, inside.p1.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint inset frame */
PCUT_TEST(inset_frame)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;
	gfx_rect_t inside;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint inset frame with NULL 'inside' output parameter */
	rc = ui_paint_inset_frame(resource, &rect, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Paint inset frame with valid 'inside' output parameter */
	rc = ui_paint_inset_frame(resource, &rect, &inside);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Get inset frame inside */
PCUT_TEST(get_inset_frame_inside)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;
	gfx_rect_t inside;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	ui_paint_get_inset_frame_inside(resource, &rect, &inside);
	PCUT_ASSERT_INT_EQUALS(12, inside.p0.x);
	PCUT_ASSERT_INT_EQUALS(22, inside.p0.y);
	PCUT_ASSERT_INT_EQUALS(28, inside.p1.x);
	PCUT_ASSERT_INT_EQUALS(38, inside.p1.y);

	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint filled circle */
PCUT_TEST(filled_circle)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	/* Paint filled circle / upper-left half */
	rc = ui_paint_filled_circle(gc, &center, 10, ui_fcircle_upleft);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Paint filled circle / lower-right half */
	rc = ui_paint_filled_circle(gc, &center, 10, ui_fcircle_lowright);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Paint entire filled circle */
	rc = ui_paint_filled_circle(gc, &center, 10, ui_fcircle_entire);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint up pointing triangle */
PCUT_TEST(up_triangle)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_up_triangle(gc, &center, 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint down pointing triangle */
PCUT_TEST(down_triangle)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_down_triangle(gc, &center, 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint left pointing triangle */
PCUT_TEST(left_triangle)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_left_triangle(gc, &center, 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint right pointing triangle */
PCUT_TEST(right_triangle)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_right_triangle(gc, &center, 5);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint diagonal cross (X) */
PCUT_TEST(cross)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_cross(gc, &center, 5, 1, 2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint mimimize icon */
PCUT_TEST(minicon)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_minicon(resource, &center, 8, 6);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint maximize icon */
PCUT_TEST(maxicon)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_maxicon(resource, &center, 8, 6);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint unmaximize icon */
PCUT_TEST(unmaxicon)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	test_gc_t tgc;
	gfx_coord2_t center;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	center.x = 0;
	center.y = 0;

	rc = ui_paint_unmaxicon(resource, &center, 8, 8, 3, 3);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint text box */
PCUT_TEST(text_box)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	gfx_color_t *color = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint text box */
	rc = ui_paint_text_box(resource, &rect, ui_box_single, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint custom text box */
PCUT_TEST(text_box_custom)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	gfx_color_t *color = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint text box */
	rc = ui_paint_text_box_custom(resource, &rect, &test_box_chars, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint text horizontal brace */
PCUT_TEST(text_hbrace)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	gfx_color_t *color = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint text horizontal brace */
	rc = ui_paint_text_hbrace(resource, &rect, ui_box_single,
	    color);

	gfx_color_delete(color);
	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint text rectangle */
PCUT_TEST(text_rect)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	ui_resource_t *resource = NULL;
	gfx_color_t *color = NULL;
	test_gc_t tgc;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = gfx_color_new_rgb_i16(1, 2, 3, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;

	/* Paint text box */
	rc = ui_paint_text_rect(resource, &rect, color, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_color_delete(color);
	ui_resource_destroy(resource);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_EXPORT(paint);
