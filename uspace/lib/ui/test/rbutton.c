/*
 * Copyright (c) 2021 Jiri Svoboda
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
#include <ui/control.h>
#include <ui/rbutton.h>
#include <ui/resource.h>
#include "../private/rbutton.h"

PCUT_INIT;

PCUT_TEST_SUITE(rbutton);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_update(void *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.update = testgc_update,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

static void test_rbutton_select(ui_rbutton_group_t *, void *, void *);

static ui_rbutton_group_cb_t test_rbutton_group_cb = {
	.selected = test_rbutton_select
};

static ui_rbutton_group_cb_t dummy_rbutton_group_cb = {
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
	bool selected;
} test_cb_resp_t;

/** Create and destroy radio button */
PCUT_TEST(create_destroy)
{
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton = NULL;
	errno_t rc;

	rc = ui_rbutton_group_create(NULL, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(rbutton);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
}

/** ui_rbutton_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_rbutton_destroy(NULL);
}

/** ui_rbutton_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton;
	ui_control_t *control;
	errno_t rc;

	rc = ui_rbutton_group_create(NULL, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_rbutton_ctl(rbutton);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
	ui_rbutton_group_destroy(group);
}

/** Set radio button rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_rbutton_group_create(NULL, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_rbutton_set_rect(rbutton, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, rbutton->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, rbutton->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, rbutton->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, rbutton->rect.p1.y);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
}

/** Paint radio button in graphics mode */
PCUT_TEST(paint_gfx)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_rbutton_group_t *group = NULL;
	ui_resource_t *resource = NULL;
	ui_rbutton_t *rbutton;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_paint_gfx(rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Paint radio button in text mode */
PCUT_TEST(paint_text)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_rbutton_group_t *group = NULL;
	ui_resource_t *resource = NULL;
	ui_rbutton_t *rbutton;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_paint_text(rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test ui_rbutton_selected() */
PCUT_TEST(selected)
{
	errno_t rc;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton;
	test_cb_resp_t resp;

	rc = ui_rbutton_group_create(NULL, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Selected with no callbacks set */
	ui_rbutton_selected(rbutton);

	/* Selected with callback not implementing selected */
	ui_rbutton_group_set_cb(group, &dummy_rbutton_group_cb, NULL);
	ui_rbutton_selected(rbutton);

	/* Selected with real callback set */
	resp.selected = false;
	ui_rbutton_group_set_cb(group, &test_rbutton_group_cb, &resp);
	ui_rbutton_selected(rbutton);
	PCUT_ASSERT_TRUE(resp.selected);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
}

/** Press and release radio button */
PCUT_TEST(press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton1;
	ui_rbutton_t *rbutton2;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NULL(group->selected);

	rc = ui_rbutton_create(group, "One", NULL, &rbutton1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	rc = ui_rbutton_create(group, "Two", NULL, &rbutton2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	resp.selected = false;
	ui_rbutton_group_set_cb(group, &test_rbutton_group_cb, &resp);

	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);

	ui_rbutton_press(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_release(rbutton2);
	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_TRUE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton2);

	ui_rbutton_destroy(rbutton1);
	ui_rbutton_destroy(rbutton2);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, leave and release radio button */
PCUT_TEST(press_leave_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton1;
	ui_rbutton_t *rbutton2;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NULL(group->selected);

	rc = ui_rbutton_create(group, "One", NULL, &rbutton1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	rc = ui_rbutton_create(group, "Two", NULL, &rbutton2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	resp.selected = false;
	ui_rbutton_group_set_cb(group, &test_rbutton_group_cb, &resp);

	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);

	ui_rbutton_press(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_leave(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_release(rbutton2);
	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_destroy(rbutton1);
	ui_rbutton_destroy(rbutton2);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Press, leave, enter and release radio button */
PCUT_TEST(press_leave_enter_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton1;
	ui_rbutton_t *rbutton2;
	test_cb_resp_t resp;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NULL(group->selected);

	rc = ui_rbutton_create(group, "One", NULL, &rbutton1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	rc = ui_rbutton_create(group, "Two", NULL, &rbutton2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	resp.selected = false;
	ui_rbutton_group_set_cb(group, &test_rbutton_group_cb, &resp);

	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);

	ui_rbutton_press(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_leave(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_FALSE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_enter(rbutton2);
	PCUT_ASSERT_TRUE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_FALSE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton1);

	ui_rbutton_release(rbutton2);
	PCUT_ASSERT_FALSE(rbutton2->held);
	PCUT_ASSERT_TRUE(rbutton2->inside);
	PCUT_ASSERT_TRUE(resp.selected);
	PCUT_ASSERT_EQUALS(group->selected, rbutton2);

	ui_rbutton_destroy(rbutton1);
	ui_rbutton_destroy(rbutton2);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_pos_event() correctly translates POS_PRESS/POS_RELEASE */
PCUT_TEST(pos_event_press_release)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton;
	ui_evclaim_t claim;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(rbutton->held);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_rbutton_set_rect(rbutton, &rect);

	/* Press outside is not claimed and does nothing */
	event.type = POS_PRESS;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_FALSE(rbutton->held);
	PCUT_ASSERT_EQUALS(ui_unclaimed, claim);

	/* Press inside is claimed and depresses radio button */
	event.type = POS_PRESS;
	event.hpos = 10;
	event.vpos = 20;
	claim = ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_TRUE(rbutton->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	/* Release outside (or anywhere) is claimed and relases radio button */
	event.type = POS_RELEASE;
	event.hpos = 9;
	event.vpos = 20;
	claim = ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_FALSE(rbutton->held);
	PCUT_ASSERT_EQUALS(ui_claimed, claim);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** ui_pos_event() correctly translates POS_UPDATE to enter/leave */
PCUT_TEST(pos_event_enter_leave)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_rbutton_group_t *group = NULL;
	ui_rbutton_t *rbutton;
	pos_event_t event;
	gfx_rect_t rect;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_rbutton_group_create(resource, &group);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_rbutton_create(group, "Hello", NULL, &rbutton);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(rbutton->inside);

	rect.p0.x = 10;
	rect.p0.y = 20;
	rect.p1.x = 30;
	rect.p1.y = 40;
	ui_rbutton_set_rect(rbutton, &rect);

	/* Moving outside does nothing */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_FALSE(rbutton->inside);

	/* Moving inside sets inside flag */
	event.type = POS_UPDATE;
	event.hpos = 10;
	event.vpos = 20;
	ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_TRUE(rbutton->inside);

	/* Moving outside clears inside flag */
	event.type = POS_UPDATE;
	event.hpos = 9;
	event.vpos = 20;
	ui_rbutton_pos_event(rbutton, &event);
	PCUT_ASSERT_FALSE(rbutton->inside);

	ui_rbutton_destroy(rbutton);
	ui_rbutton_group_destroy(group);
	ui_resource_destroy(resource);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	(void) arg;
	(void) rect;
	return EOK;
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

static errno_t testgc_update(void *arg)
{
	(void) arg;
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

static void test_rbutton_select(ui_rbutton_group_t *group, void *arg,
    void *barg)
{
	test_cb_resp_t *resp = (test_cb_resp_t *) arg;

	resp->selected = true;
}

PCUT_EXPORT(rbutton);
