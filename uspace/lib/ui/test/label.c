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
#include <ui/label.h>
#include <ui/resource.h>
#include "../private/label.h"

PCUT_INIT;

PCUT_TEST_SUITE(label);

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
	bool clicked;
} test_cb_resp_t;

/** Create and destroy label */
PCUT_TEST(create_destroy)
{
	ui_label_t *label = NULL;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(label);

	ui_label_destroy(label);
}

/** ui_label_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_label_destroy(NULL);
}

/** ui_label_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_label_t *label;
	ui_control_t *control;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_label_ctl(label);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

/** Set label rectangle sets internal field */
PCUT_TEST(set_rect)
{
	ui_label_t *label;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_label_set_rect(label, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, label->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, label->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, label->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, label->rect.p1.y);

	ui_label_destroy(label);
}

/** Set label text horizontal alignment sets internal field */
PCUT_TEST(set_halign)
{
	ui_label_t *label;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_label_set_halign(label, gfx_halign_left);
	PCUT_ASSERT_EQUALS(gfx_halign_left, label->halign);
	ui_label_set_halign(label, gfx_halign_center);
	PCUT_ASSERT_EQUALS(gfx_halign_center, label->halign);

	ui_label_destroy(label);
}

/** Set label rectangle sets internal field */
PCUT_TEST(set_text)
{
	ui_label_t *label;
	gfx_rect_t rect;
	errno_t rc;

	rc = ui_label_create(NULL, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	ui_label_set_rect(label, &rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, label->rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, label->rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, label->rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, label->rect.p1.y);

	ui_label_destroy(label);
}

/** Paint label */
PCUT_TEST(paint)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	ui_resource_t *resource = NULL;
	ui_label_t *label;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_resource_create(gc, false, &resource);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(resource);

	rc = ui_label_create(resource, "Hello", &label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = ui_label_paint(label);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_label_destroy(label);
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

PCUT_EXPORT(label);
