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
#include <gfx/glyph_bmp.h>
#include <gfx/typeface.h>
#include <pcut/pcut.h>
#include <str.h>
#include "../private/font.h"
#include "../private/typeface.h"

PCUT_INIT;

PCUT_TEST_SUITE(tpf);

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

static const gfx_font_flags_t test_font_flags = gff_bold_italic;

enum {
	test_font_size = 9,
	test_font_ascent = 4,
	test_font_descent = 3,
	test_font_leading = 2,
	test_glyph_advance = 10
};

static const char *test_glyph_pattern = "ff";

/** Test saving typeface to and loading from TPF file */
PCUT_TEST(save_load)
{
	char fname[L_tmpnam];
	char *p;
	gfx_font_props_t props;
	gfx_font_props_t rprops;
	gfx_font_metrics_t metrics;
	gfx_font_metrics_t rmetrics;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_metrics_t rgmetrics;
	gfx_glyph_pattern_t *pat;
	const char *str;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_font_info_t *finfo;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	gfx_glyph_bmp_t *bmp;
	int pix;
	test_gc_t tgc;
	errno_t rc;

	p = tmpnam(fname);
	PCUT_ASSERT_NOT_NULL(p);

	rc = gfx_context_new(&test_ops, (void *)&tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&props);
	props.size = test_font_size;
	props.flags = test_font_flags;

	gfx_font_metrics_init(&metrics);
	metrics.ascent = test_font_ascent;
	metrics.descent = test_font_descent;
	metrics.leading = test_font_leading;

	rc = gfx_font_create(tface, &props, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = test_glyph_advance;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_set_pattern(glyph, test_glyph_pattern);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 1, 1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_save(bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_bmp_close(bmp);

	rc = gfx_typeface_save(tface, fname);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_typeface_destroy(tface);
	tface = NULL;

	rc = gfx_typeface_open(gc, fname, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(tface);

	finfo = gfx_typeface_first_font(tface);
	PCUT_ASSERT_NOT_NULL(finfo);

	gfx_font_get_props(finfo, &rprops);
	PCUT_ASSERT_INT_EQUALS(props.size, rprops.size);
	PCUT_ASSERT_INT_EQUALS(props.flags, rprops.flags);

	rc = gfx_font_open(finfo, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(font);

	gfx_font_get_metrics(font, &rmetrics);
	PCUT_ASSERT_INT_EQUALS(metrics.ascent, rmetrics.ascent);
	PCUT_ASSERT_INT_EQUALS(metrics.descent, rmetrics.descent);
	PCUT_ASSERT_INT_EQUALS(metrics.leading, rmetrics.leading);

	glyph = gfx_font_first_glyph(font);
	PCUT_ASSERT_NOT_NULL(glyph);

	gfx_glyph_get_metrics(glyph, &rgmetrics);
	PCUT_ASSERT_INT_EQUALS(gmetrics.advance, rgmetrics.advance);

	pat = gfx_glyph_first_pattern(glyph);
	str = gfx_glyph_pattern_str(pat);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp(test_glyph_pattern, str));

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(1, pix);
	pix = gfx_glyph_bmp_getpix(bmp, 1, 1);
	PCUT_ASSERT_INT_EQUALS(1, pix);
	pix = gfx_glyph_bmp_getpix(bmp, 1, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);
	pix = gfx_glyph_bmp_getpix(bmp, 0, 1);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	gfx_glyph_bmp_close(bmp);

	gfx_typeface_destroy(tface);
	tface = NULL;

	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	(void) remove(p);
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

PCUT_EXPORT(tpf);
