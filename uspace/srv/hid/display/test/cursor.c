/*
 * Copyright (c) 2022 Jiri Svoboda
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

#include <errno.h>
#include <gfx/context.h>
#include <stdbool.h>
#include <pcut/pcut.h>

#include "../cursor.h"
#include "../cursimg.h"
#include "../ddev.h"
#include "../display.h"
#include "../display.h"

PCUT_INIT;

PCUT_TEST_SUITE(cursor);

static errno_t dummy_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t dummy_bitmap_destroy(void *);
static errno_t dummy_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t dummy_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t dummy_ops = {
	.bitmap_create = dummy_bitmap_create,
	.bitmap_destroy = dummy_bitmap_destroy,
	.bitmap_render = dummy_bitmap_render,
	.bitmap_get_alloc = dummy_bitmap_get_alloc
};

typedef struct {
	bool render_called;
} test_response_t;

typedef struct {
	test_response_t *resp;
	gfx_bitmap_alloc_t alloc;
} dummy_bitmap_t;

/** Test ds_cursor_create(), ds_cursor_destroy(). */
PCUT_TEST(cursor_create_destroy)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

/** Test ds_cursor_paint() renders the cursor. */
PCUT_TEST(cursor_paint_render)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_cursor_t *cursor;
	ds_ddev_t *ddev;
	ddev_info_t ddinfo;
	gfx_coord2_t pos;
	test_response_t resp;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, &resp, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ddev_info_init(&ddinfo);

	rc = ds_ddev_create(disp, NULL, &ddinfo, NULL, 0, gc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.render_called = false;

	pos.x = 0;
	pos.y = 0;
	ds_cursor_paint(cursor, &pos, NULL);

	PCUT_ASSERT_TRUE(resp.render_called);

	ds_ddev_close(ddev);
	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

/** Test ds_cursor_paint() optimizes out rendering using clipping rectangle. */
PCUT_TEST(cursor_paint_norender)
{
	gfx_context_t *gc;
	ds_display_t *disp;
	ds_cursor_t *cursor;
	ds_ddev_t *ddev;
	ddev_info_t ddinfo;
	gfx_coord2_t pos;
	gfx_rect_t clip;
	test_response_t resp;
	errno_t rc;

	rc = gfx_context_new(&dummy_ops, &resp, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_display_create(gc, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ddev_info_init(&ddinfo);

	rc = ds_ddev_create(disp, NULL, &ddinfo, NULL, 0, gc, &ddev);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	resp.render_called = false;

	/*
	 * Clipping rectangle not intersecting the area where cursor is to
	 * be rendered
	 */
	clip.p0.x = 100;
	clip.p0.y = 100;
	clip.p1.x = 150;
	clip.p1.y = 150;

	pos.x = 0;
	pos.y = 0;
	ds_cursor_paint(cursor, &pos, &clip);

	/* Rendering should have been optimized out */
	PCUT_ASSERT_FALSE(resp.render_called);

	ds_ddev_close(ddev);
	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

/** Test ds_cursor_get_rect() */
PCUT_TEST(cursor_get_rect)
{
	ds_display_t *disp;
	ds_cursor_t *cursor;
	gfx_coord2_t pos1, pos2;
	gfx_rect_t rect1, rect2;
	errno_t rc;

	rc = ds_display_create(NULL, df_none, &disp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ds_cursor_create(disp, &ds_cursimg[dcurs_arrow].rect,
	    ds_cursimg[dcurs_arrow].image, &cursor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos1.x = 10;
	pos1.y = 11;

	pos2.x = 22;
	pos2.y = 23;

	ds_cursor_get_rect(cursor, &pos1, &rect1);
	ds_cursor_get_rect(cursor, &pos2, &rect2);

	PCUT_ASSERT_FALSE(gfx_rect_is_empty(&rect1));
	PCUT_ASSERT_FALSE(gfx_rect_is_empty(&rect2));

	PCUT_ASSERT_INT_EQUALS(pos2.x - pos1.x, rect2.p0.x - rect1.p0.x);
	PCUT_ASSERT_INT_EQUALS(pos2.y - pos1.y, rect2.p0.y - rect1.p0.y);
	PCUT_ASSERT_INT_EQUALS(pos2.x - pos1.x, rect2.p1.x - rect1.p1.x);
	PCUT_ASSERT_INT_EQUALS(pos2.y - pos1.y, rect2.p1.y - rect1.p1.y);

	ds_cursor_destroy(cursor);
	ds_display_destroy(disp);
}

static errno_t dummy_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	test_response_t *resp = (test_response_t *) arg;
	dummy_bitmap_t *bm;
	gfx_coord2_t dims;

	gfx_rect_dims(&params->rect, &dims);
	bm = calloc(1, sizeof(dummy_bitmap_t));
	if (bm == NULL)
		return ENOMEM;

	bm->resp = resp;
	bm->alloc.pitch = dims.x * sizeof(uint32_t);
	bm->alloc.off0 = 0;

	bm->alloc.pixels = malloc(bm->alloc.pitch * dims.y * sizeof(uint32_t));
	if (bm->alloc.pixels == NULL) {
		free(bm);
		return ENOMEM;
	}

	*rbm = (void *) bm;
	return EOK;
}

static errno_t dummy_bitmap_destroy(void *arg)
{
	dummy_bitmap_t *bm = (dummy_bitmap_t *) arg;

	free(bm);
	return EOK;
}

static errno_t dummy_bitmap_render(void *arg, gfx_rect_t *rect,
    gfx_coord2_t *dpos)
{
	dummy_bitmap_t *bm = (dummy_bitmap_t *) arg;

	bm->resp->render_called = true;
	return EOK;
}

static errno_t dummy_bitmap_get_alloc(void *arg, gfx_bitmap_alloc_t *alloc)
{
	dummy_bitmap_t *bm = (dummy_bitmap_t *) arg;

	*alloc = bm->alloc;
	return EOK;
}

PCUT_EXPORT(cursor);
