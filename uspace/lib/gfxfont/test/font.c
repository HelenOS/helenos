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
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/typeface.h>
#include <pcut/pcut.h>
#include "../private/font.h"
#include "../private/typeface.h"

PCUT_INIT;

PCUT_TEST_SUITE(font);

static errno_t testgc_set_clip_rect(void *, gfx_rect_t *);
static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);
static errno_t testgc_bitmap_create(void *, gfx_bitmap_params_t *,
    gfx_bitmap_alloc_t *, void **);
static errno_t testgc_bitmap_destroy(void *);
static errno_t testgc_bitmap_render(void *, gfx_rect_t *, gfx_coord2_t *);
static errno_t testgc_bitmap_get_alloc(void *, gfx_bitmap_alloc_t *);

static gfx_context_ops_t test_ops = {
	.set_clip_rect = testgc_set_clip_rect,
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect,
	.bitmap_create = testgc_bitmap_create,
	.bitmap_destroy = testgc_bitmap_destroy,
	.bitmap_render = testgc_bitmap_render,
	.bitmap_get_alloc = testgc_bitmap_get_alloc
};

typedef struct {
	gfx_bitmap_params_t bm_params;
	void *bm_pixels;
	gfx_rect_t bm_srect;
	gfx_coord2_t bm_offs;
} test_gc_t;

typedef struct {
	test_gc_t *tgc;
	gfx_bitmap_alloc_t alloc;
	bool myalloc;
} testgc_bitmap_t;

/** Test creating and destroying font */
PCUT_TEST(create_destroy)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test creating and destroying text-mode font */
PCUT_TEST(create_textmode_destroy)
{
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_font_create_textmode(tface, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_get_metrics() */
PCUT_TEST(get_metrics)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_font_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);
	metrics.ascent = 1;
	metrics.descent = 2;
	metrics.leading = 3;

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_get_metrics(font, &gmetrics);
	PCUT_ASSERT_INT_EQUALS(metrics.ascent, gmetrics.ascent);
	PCUT_ASSERT_INT_EQUALS(metrics.descent, gmetrics.descent);
	PCUT_ASSERT_INT_EQUALS(metrics.leading, gmetrics.leading);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_set_metrics() */
PCUT_TEST(set_metrics)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics1;
	gfx_font_metrics_t metrics2;
	gfx_font_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);

	gfx_font_metrics_init(&metrics1);
	metrics1.ascent = 1;
	metrics1.descent = 2;
	metrics1.leading = 3;

	rc = gfx_font_create(tface, &props, &metrics1, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics2);
	metrics1.ascent = 4;
	metrics1.descent = 5;
	metrics1.leading = 6;

	rc = gfx_font_set_metrics(font, &metrics2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_get_metrics(font, &gmetrics);
	PCUT_ASSERT_INT_EQUALS(metrics2.ascent, gmetrics.ascent);
	PCUT_ASSERT_INT_EQUALS(metrics2.descent, gmetrics.descent);
	PCUT_ASSERT_INT_EQUALS(metrics2.leading, gmetrics.leading);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_first_glyph() */
PCUT_TEST(first_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	gfx_glyph_t *gfirst;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Should get NULL since there is no glyph in the font */
	glyph = gfx_font_first_glyph(font);
	PCUT_ASSERT_NULL(glyph);

	/* Now add one */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* gfx_font_first_glyph() should return the same */
	gfirst = gfx_font_first_glyph(font);
	PCUT_ASSERT_EQUALS(glyph, gfirst);

	gfx_glyph_destroy(glyph);
	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_next_glyph() */
PCUT_TEST(next_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph1;
	gfx_glyph_t *glyph2;
	gfx_glyph_t *gfirst;
	gfx_glyph_t *gsecond;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add first glyph */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add second glyph */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* gfx_font_first_glyph() should return glyph1 */
	gfirst = gfx_font_first_glyph(font);
	PCUT_ASSERT_EQUALS(glyph1, gfirst);

	/* gfx_font_next_glyph() should return glyph2 */
	gsecond = gfx_font_next_glyph(gfirst);
	PCUT_ASSERT_EQUALS(glyph2, gsecond);

	gfx_glyph_destroy(glyph1);
	gfx_glyph_destroy(glyph2);
	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_last_glyph() */
PCUT_TEST(last_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	gfx_glyph_t *glast;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Should get NULL since there is no glyph in the font */
	glyph = gfx_font_last_glyph(font);
	PCUT_ASSERT_NULL(glyph);

	/* Now add one */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* gfx_font_last_glyph() should return the same */
	glast = gfx_font_last_glyph(font);
	PCUT_ASSERT_EQUALS(glyph, glast);

	gfx_glyph_destroy(glyph);
	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_prev_glyph() */
PCUT_TEST(prev_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph1;
	gfx_glyph_t *glyph2;
	gfx_glyph_t *gfirst;
	gfx_glyph_t *gsecond;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add first glyph */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Add second glyph */
	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph2);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* gfx_font_last_glyph() should return glyph2 */
	gsecond = gfx_font_last_glyph(font);
	PCUT_ASSERT_EQUALS(glyph2, gsecond);

	/* gfx_font_prev_glyph() should return glyph1 */
	gfirst = gfx_font_prev_glyph(gsecond);
	PCUT_ASSERT_EQUALS(glyph1, gfirst);

	gfx_glyph_destroy(glyph1);
	gfx_glyph_destroy(glyph2);
	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_search_glyph() */
PCUT_TEST(search_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	size_t bytes;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_font_search_glyph(font, "Hello", &glyph, &bytes);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_splice_at_glyph() */
PCUT_TEST(splice_at_glyph)
{
	gfx_font_props_t props;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_rect_t nrect;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &props, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	nrect.p0.x = -5;
	nrect.p0.y = -5;
	nrect.p1.x = 5;
	nrect.p1.y = 5;
	rc = gfx_font_splice_at_glyph(font, glyph, &nrect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_bitmap_pack() properly packs bitmap */
PCUT_TEST(bitmap_pack)
{
	errno_t rc;
	gfx_coord_t width, height;
	gfx_coord_t i;
	uint32_t *pixels;
	void *data;
	size_t size;
	uint8_t *dp;

	width = 10;
	height = 10;

	pixels = calloc(sizeof(uint32_t), width * height);
	PCUT_ASSERT_NOT_NULL(pixels);

	for (i = 0; i < 10; i++)
		pixels[i * width + i] = PIXEL(255, 255, 255, 255);

	rc = gfx_font_bitmap_pack(width, height, pixels, &data, &size);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(data);
	PCUT_ASSERT_INT_EQUALS(20, size);

	dp = data;

	for (i = 0; i < 8; i++) {
		PCUT_ASSERT_INT_EQUALS(0x80 >> i, dp[2 * i]);
		PCUT_ASSERT_INT_EQUALS(0, dp[2 * i + 1]);
	}

	for (i = 8; i < 10; i++) {
		PCUT_ASSERT_INT_EQUALS(0, dp[2 * i]);
		PCUT_ASSERT_INT_EQUALS(0x80 >> (i - 8), dp[2 * i + 1]);
	}
}

/** Test gfx_font_bitmap_unpack() properly unpacks bitmap */
PCUT_TEST(bitmap_unpack)
{
	errno_t rc;
	gfx_coord_t width, height;
	gfx_coord_t i, j;
	uint32_t epix;
	uint32_t *pixels;
	uint8_t data[20];

	width = 10;
	height = 10;

	for (i = 0; i < 8; i++) {
		data[2 * i] = 0x80 >> i;
		data[2 * i + 1] = 0;
	}

	for (i = 8; i < 10; i++) {
		data[2 * i] = 0;
		data[2 * i + 1] = 0x80 >> (i - 8);
	}

	pixels = calloc(sizeof(uint32_t), width * height);
	PCUT_ASSERT_NOT_NULL(pixels);

	rc = gfx_font_bitmap_unpack(width, height, data, sizeof(data),
	    pixels);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	for (j = 0; j < 10; j++) {
		for (i = 0; i < 10; i++) {
			epix = (i == j) ? PIXEL(255, 255, 255, 255) :
			    PIXEL(0, 0, 0, 0);
			PCUT_ASSERT_INT_EQUALS(epix, pixels[j * width + i]);
		}
	}
}

static errno_t testgc_set_clip_rect(void *arg, gfx_rect_t *rect)
{
	return EOK;
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	return EOK;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
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
		    tbm->alloc.pitch * (params->rect.p1.y - params->rect.p0.y));
		tbm->myalloc = true;
		if (tbm->alloc.pixels == NULL) {
			free(tbm);
			return ENOMEM;
		}
	} else {
		tbm->alloc = *alloc;
	}

	tbm->tgc = tgc;
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
	free(tbm);
	return EOK;
}

static errno_t testgc_bitmap_render(void *bm, gfx_rect_t *srect,
    gfx_coord2_t *offs)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	tbm->tgc->bm_srect = *srect;
	tbm->tgc->bm_offs = *offs;
	return EOK;
}

static errno_t testgc_bitmap_get_alloc(void *bm, gfx_bitmap_alloc_t *alloc)
{
	testgc_bitmap_t *tbm = (testgc_bitmap_t *)bm;
	*alloc = tbm->alloc;
	return EOK;
}

PCUT_EXPORT(font);
