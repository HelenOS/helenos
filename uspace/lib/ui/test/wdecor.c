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

#include <gfx/context.h>
#include <gfx/coord.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <ui/resource.h>
#include <ui/wdecor.h>
#include "../private/wdecor.h"

PCUT_INIT;

PCUT_TEST_SUITE(wdecor);

static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

static void test_wdecor_move(ui_wdecor_t *, void *, gfx_coord2_t *);

static ui_wdecor_cb_t test_wdecor_cb = {
	.move = test_wdecor_move
};

static ui_wdecor_cb_t dummy_wdecor_cb = {
};

typedef struct {
	bool bm_created;
	bool bm_destroyed;
	gfx_bitmap_params_t bm_params;
	void *bm_pixels;
	gfx_rect_t bm_srect;
	gfx_coord2_t bm_offs;
	bool bm_rendered;
	bool bm_got_alloc;
} test_gc_t;

typedef struct {
	test_gc_t *tgc;
	gfx_bitmap_alloc_t alloc;
	bool myalloc;
} testgc_bitmap_t;

typedef struct {
	bool move;
	gfx_coord2_t pos;
} test_cb_resp_t;

/** Create and destroy button */
PCUT_TEST(create_destroy)
{
	ui_wdecor_t *wdecor = NULL;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(wdecor);

	ui_wdecor_destroy(wdecor);
}

/** ui_wdecor_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_wdecor_destroy(NULL);
}

/** Set window decoration rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_wdecor_set_rect(wdecor, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, wdecor->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, wdecor->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, wdecor->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, wdecor->rect.p1.y);

	ui_wdecor_destroy(wdecor);
}

/** Set window decoration active sets internal field */
PCUT_TEST(set_active)
{
	ui_wdecor_t *wdecor;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_set_active(wdecor, false);
	PCUT_ASSERT_FALSE(wdecor->active);

	ui_wdecor_set_active(wdecor, true);
	PCUT_ASSERT_TRUE(wdecor->active);

	ui_wdecor_destroy(wdecor);
}

/** Paint button */
PCUT_TEST(paint)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_wdecor_t *wdecor;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_wdecor_create(resource, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_wdecor_paint(wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_wdecor_destroy(wdecor);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_wdecor_move() */
PCUT_TEST(move)
{
	errno_t rc;
	ui_wdecor_t *wdecor;
	test_cb_resp_t resp;
	gfx_coord2_t pos;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pos.x = 3;
	pos.y = 4;

	/* Move callback with no callbacks set */
	ui_wdecor_move(wdecor, &pos);

	/* Move callback with move callback not implemented */
	ui_wdecor_set_cb(wdecor, &dummy_wdecor_cb, NULL);
	ui_wdecor_move(wdecor, &pos);

	/* Move callback with real callback set */
	resp.move = false;
	resp.pos.x = 0;
	resp.pos.y = 0;
	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, &resp);
	ui_wdecor_move(wdecor, &pos);
	PCUT_ASSERT_TRUE(resp.move);
	PCUT_ASSERT_INT_EQUALS(pos.x, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(pos.y, resp.pos.y);

	ui_wdecor_destroy(wdecor);
}

/** Button press on title bar generates move callback */
PCUT_TEST(pos_event_move)
{
	ui_wdecor_t *wdecor;
	gfx_rect_t rect;
	pos_event_t event;
	test_cb_resp_t resp;
	errno_t rc;

	rc = ui_wdecor_create(NULL, "Hello", &wdecor);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 100;
	rect.p1.y = 200;

	ui_wdecor_set_rect(wdecor, &rect);

	ui_wdecor_set_cb(wdecor, &test_wdecor_cb, (void *) &resp);

	resp.move = false;
	resp.pos.x = 0;
	resp.pos.y = 0;

	event.type = POS_PRESS;
	event.hpos = 50;
	event.vpos = 25;
	ui_wdecor_pos_event(wdecor, &event);

	PCUT_ASSERT_TRUE(resp.move);
	PCUT_ASSERT_INT_EQUALS(event.hpos, resp.pos.x);
	PCUT_ASSERT_INT_EQUALS(event.vpos, resp.pos.y);

	ui_wdecor_destroy(wdecor);
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	(void) arg;
	(void) color;
	return EOK;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
}

static errno_t testgc_bitmap_create(void *arg, gfx_bitmap_params_t *params,
    gfx_bitmap_alloc_t *alloc, void **rbm)
{
	test_gc_t *tgc = (test_gc_t *) arg;
	testgc_bitmap_t *tbm;

	tbm = calloc(1, sizeof(testgc_bitmap_t));
	if (tbm == NULL)
		return ENOMEM;

	if (alloc == NULL) {
		tbm->alloc.pitch = (params->rect.p1.x - params->rect.p0.x) *
		    sizeof(uint32_t);
		tbm->alloc.off0 = 0;
		tbm->alloc.pixels = calloc(sizeof(uint32_t),
		    (params->rect.p1.x - params->rect.p0.x) *
		    (params->rect.p1.y - params->rect.p0.y));
		tbm->myalloc = true;
		if (tbm->alloc.pixels == NULL) {
			free(tbm);
			return ENOMEM;
		}
	} else {
		tbm->alloc = *alloc;
	}

	tbm->tgc = tgc;
	tgc->bm_created = true;
	tgc->bm_params = *params;
	tgc->bm_pixels = tbm->alloc.pixels;
	*rbm = (void *)tbm;
	return EOK;
}

static errno_t testgc_bitmap_destroy(void *bm)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	if (tbm->myalloc)
		free(tbm->alloc.pixels);
	tbm->tgc->bm_destroyed = true;
	free(tbm);
	return EOK;
}

static errno_t testgc_bitmap_render(void *bm, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	tbm->tgc->bm_rendered = true;
	tbm->tgc->bm_srect = *srect;
	tbm->tgc->bm_offs = *offs;
	return EOK;
}

static errno_t testgc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	*alloc = tbm->alloc;
	tbm->tgc->bm_got_alloc = true;
	return EOK;
}

static void test_wdecor_move(ui_wdecor_t *wdecor, void *arg, gfx_coord2_t *pos)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->move = true;
	resp->pos = *pos;
}

PCUT_EXPORT(wdecor);
