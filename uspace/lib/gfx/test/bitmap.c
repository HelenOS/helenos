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

#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <mem.h>
#include <pcut/pcut.h>
#include <stdbool.h>

PCUT_INIT;

PCUT_TEST_SUITE(bitmap);

static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t ops = {
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

enum {
	alloc_pitch = 42,
	alloc_off0 = 33
};

PCUT_TEST(create_destroy)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap = NULL;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 1;
	params.rect.p0.y = 2;
	params.rect.p1.x = 3;
	params.rect.p1.y = 4;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bitmap);
	PCUT_ASSERT_TRUE(tgc.bm_created);
	PCUT_ASSERT_EQUALS(params.rect.p0.x, tgc.bm_params.rect.p0.x);
	PCUT_ASSERT_EQUALS(params.rect.p0.y, tgc.bm_params.rect.p0.y);
	PCUT_ASSERT_EQUALS(params.rect.p1.x, tgc.bm_params.rect.p1.x);
	PCUT_ASSERT_EQUALS(params.rect.p1.y, tgc.bm_params.rect.p1.y);

	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(tgc.bm_destroyed);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_TEST(render)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap = NULL;
	gfx_rect_t srect;
	gfx_coord2_t offs;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_bitmap_params_init(&params);

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	srect.p0.x = 1;
	srect.p0.y = 2;
	srect.p1.x = 3;
	srect.p1.y = 4;
	offs.x = 5;
	offs.y = 6;

	rc = gfx_bitmap_render(bitmap, &srect, &offs);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(tgc.bm_rendered);
	PCUT_ASSERT_EQUALS(srect.p0.x, tgc.bm_srect.p0.x);
	PCUT_ASSERT_EQUALS(srect.p0.y, tgc.bm_srect.p0.y);
	PCUT_ASSERT_EQUALS(srect.p1.x, tgc.bm_srect.p1.x);
	PCUT_ASSERT_EQUALS(srect.p1.y, tgc.bm_srect.p1.y);
	PCUT_ASSERT_EQUALS(offs.x, tgc.bm_offs.x);
	PCUT_ASSERT_EQUALS(offs.y, tgc.bm_offs.y);

	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_TEST(get_alloc)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap = NULL;
	gfx_bitmap_alloc_t alloc;

	memset(&tgc, 0, sizeof(tgc));
	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_bitmap_params_init(&params);

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(tgc.bm_got_alloc);
	PCUT_ASSERT_EQUALS(alloc_pitch, alloc.pitch);
	PCUT_ASSERT_EQUALS(alloc_off0, alloc.off0);
	PCUT_ASSERT_EQUALS(tgc.bm_pixels, alloc.pixels);

	rc = gfx_bitmap_destroy(bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
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
		tbm->alloc.pitch = alloc_pitch;
		tbm->alloc.off0 = alloc_off0;
		tbm->alloc.pixels = calloc(1, 420);
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

PCUT_EXPORT(bitmap);
