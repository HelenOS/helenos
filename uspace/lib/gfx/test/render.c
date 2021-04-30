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

#include <gfx/color.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <pcut/pcut.h>
#include <mem.h>
#include <stdbool.h>

PCUT_INIT;

PCUT_TEST_SUITE(render);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_update(void *);

static gfx_context_ops_t ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.update = testgc_update
};

/** Test graphics context data */
typedef struct {
	errno_t rc;

	bool set_clip_rect;
	gfx_rect_t crect;
	bool do_clip;

	bool set_color;
	gfx_color_t *dclr;

	bool fill_rect;
	gfx_rect_t frect;

	bool update;
} test_gc_t;

/** Set clipping rectangle */
PCUT_TEST(set_clip_rect)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	tgc.rc = EOK;

	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.set_clip_rect);
	PCUT_ASSERT_TRUE(tgc.do_clip);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc.crect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc.crect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc.crect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc.crect.p1.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set null clipping rectangle */
PCUT_TEST(set_clip_rect_null)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EOK;

	rc = gfx_set_clip_rect(gc, NULL);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.set_clip_rect);
	PCUT_ASSERT_FALSE(tgc.do_clip);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set clipping rectangle with error return */
PCUT_TEST(set_clip_rect_failure)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EIO;

	rc = gfx_set_clip_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set drawing color */
PCUT_TEST(set_color)
{
	errno_t rc;
	gfx_color_t *color;
	gfx_context_t *gc = NULL;
	uint16_t r, g, b;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(tgc.set_color);

	tgc.rc = EOK;

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.set_color);

	gfx_color_get_rgb_i16(tgc.dclr, &r, &g, &b);

	PCUT_ASSERT_INT_EQUALS(0xffff, r);
	PCUT_ASSERT_INT_EQUALS(0xffff, g);
	PCUT_ASSERT_INT_EQUALS(0xffff, b);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Set drawing color with error return */
PCUT_TEST(set_color_failure)
{
	errno_t rc;
	gfx_color_t *color;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_color_new_rgb_i16(0xffff, 0xffff, 0xffff, &color);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_FALSE(tgc.set_color);

	tgc.rc = EIO;

	rc = gfx_set_color(gc, color);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	gfx_color_delete(color);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Fill rectangle */
PCUT_TEST(fill_rect)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	PCUT_ASSERT_FALSE(tgc.fill_rect);

	tgc.rc = EOK;

	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_TRUE(tgc.fill_rect);
	PCUT_ASSERT_INT_EQUALS(rect.p0.x, tgc.frect.p0.x);
	PCUT_ASSERT_INT_EQUALS(rect.p0.y, tgc.frect.p0.y);
	PCUT_ASSERT_INT_EQUALS(rect.p1.x, tgc.frect.p1.x);
	PCUT_ASSERT_INT_EQUALS(rect.p1.y, tgc.frect.p1.y);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Fill rectangle with error return */
PCUT_TEST(fill_rect_failure)
{
	errno_t rc;
	gfx_rect_t rect;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rect.p0.x = 1;
	rect.p0.y = 2;
	rect.p1.x = 3;
	rect.p1.y = 4;

	PCUT_ASSERT_FALSE(tgc.fill_rect);

	tgc.rc = EIO;

	rc = gfx_fill_rect(gc, &rect);
	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Update GC */
PCUT_TEST(update)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EOK;

	PCUT_ASSERT_FALSE(tgc.update);
	rc = gfx_update(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_TRUE(tgc.update);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Update GC with error return */
PCUT_TEST(update_failure)
{
	errno_t rc;
	gfx_context_t *gc = NULL;
	test_gc_t tgc;

	memset(&tgc, 0, sizeof(tgc));

	rc = gfx_context_new(&ops, &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tgc.rc = EIO;
	rc = gfx_update(gc);

	PCUT_ASSERT_ERRNO_VAL(EIO, rc);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->set_clip_rect = true;
	if (rect != NULL) {
		tgc->do_clip = true;
		tgc->crect = *rect;
	} else {
		tgc->do_clip = false;
	}

	return tgc->rc;
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->set_color = true;

	/* Technically we should copy the data */
	tgc->dclr = color;
	return tgc->rc;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->fill_rect = true;
	tgc->frect = *rect;
	return tgc->rc;
}

static errno_t testgc_update(void *arg)
{
	test_gc_t *tgc = (test_gc_t *) arg;

	tgc->update = true;
	return tgc->rc;
}

PCUT_EXPORT(render);
