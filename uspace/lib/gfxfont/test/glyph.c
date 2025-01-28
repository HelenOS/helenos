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

#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/glyph_bmp.h>
#include <gfx/typeface.h>
#include <io/pixelmap.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <str.h>
#include "../private/glyph.h"
#include "../private/testgc.h"

PCUT_INIT;

PCUT_TEST_SUITE(glyph);

/** Test creating and destroying glyph */
PCUT_TEST(create_destroy)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_get_metrics() */
PCUT_TEST(get_metrics)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_glyph_metrics_t rmetrics;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 42;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_get_metrics(glyph, &rmetrics);
	PCUT_ASSERT_INT_EQUALS(gmetrics.advance, rmetrics.advance);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_set_metrics() */
PCUT_TEST(set_metrics)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics1;
	gfx_glyph_metrics_t gmetrics2;
	gfx_glyph_t *glyph;
	gfx_glyph_metrics_t rmetrics;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics1);
	gmetrics1.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics1, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics2);
	gmetrics2.advance = 2;

	gfx_glyph_set_metrics(glyph, &gmetrics2);

	gfx_glyph_get_metrics(glyph, &rmetrics);
	PCUT_ASSERT_INT_EQUALS(gmetrics2.advance, rmetrics.advance);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_set_pattern() */
PCUT_TEST(set_pattern)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	/* Set a pattern */
	rc = gfx_glyph_set_pattern(glyph, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	/* Setting the same pattern again should be OK */
	rc = gfx_glyph_set_pattern(glyph, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_clear_pattern() */
PCUT_TEST(clear_pattern)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	/* Set a pattern */
	rc = gfx_glyph_set_pattern(glyph, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	/* Now clear a different pattern - should be OK */
	gfx_glyph_clear_pattern(glyph, "AA");
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	/* Now clear the pattern which has been set */
	gfx_glyph_clear_pattern(glyph, "A");
	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_matches() */
PCUT_TEST(matches)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	bool match;
	size_t msize;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	/* Set a pattern */
	rc = gfx_glyph_set_pattern(glyph, "AB");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	match = gfx_glyph_matches(glyph, "A", &msize);
	PCUT_ASSERT_FALSE(match);

	match = gfx_glyph_matches(glyph, "AB", &msize);
	PCUT_ASSERT_TRUE(match);
	PCUT_ASSERT_INT_EQUALS(2, msize);

	match = gfx_glyph_matches(glyph, "ABC", &msize);
	PCUT_ASSERT_TRUE(match);
	PCUT_ASSERT_INT_EQUALS(2, msize);

	match = gfx_glyph_matches(glyph, "BAB", &msize);
	PCUT_ASSERT_FALSE(match);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_first_pattern(), gfx_glyph_next_pattern() */
PCUT_TEST(first_next_pattern)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	gfx_glyph_pattern_t *pat;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	/* Set a pattern */
	rc = gfx_glyph_set_pattern(glyph, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	pat = gfx_glyph_first_pattern(glyph);
	PCUT_ASSERT_NOT_NULL(pat);

	pat = gfx_glyph_next_pattern(pat);
	PCUT_ASSERT_NULL(pat);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_pattern_str() */
PCUT_TEST(pattern_str)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	test_gc_t tgc;
	gfx_glyph_pattern_t *pat;
	const char *pstr;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_NULL(gfx_glyph_first_pattern(glyph));

	/* Set a pattern */
	rc = gfx_glyph_set_pattern(glyph, "A");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(gfx_glyph_first_pattern(glyph));

	pat = gfx_glyph_first_pattern(glyph);
	PCUT_ASSERT_NOT_NULL(pat);

	pstr = gfx_glyph_pattern_str(pat);
	PCUT_ASSERT_INT_EQUALS(0, str_cmp("A", pstr));

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_transfer() */
PCUT_TEST(transfer)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_glyph_bmp_t *bmp;
	pixelmap_t pmap;
	pixel_t pixel;
	test_gc_t tgc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, (void *) &tgc, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_typeface_create(gc, &tface);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_props_init(&fprops);
	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(tface, &fprops, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 1;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/*
	 * We need to fill some pixels of the glyph.
	 * We'll use the glyph bitmap for that.
	 * That means this test won't pass unless glyph
	 * bitmap works.
	 */

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 1, 1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_save(bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	gfx_glyph_bmp_close(bmp);

	/* Now create a new bitmap */

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 10;
	params.rect.p1.y = 10;
	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Transfer the glyph to new bitmap */
	rc = gfx_glyph_transfer(glyph, 0, bitmap, &params.rect);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Now let's read pixels from the new bitmap */
	pmap.width = params.rect.p1.x;
	pmap.height = params.rect.p1.y;
	pmap.data = alloc.pixels;

	pixel = pixelmap_get_pixel(&pmap, 0, 0);
	PCUT_ASSERT_INT_EQUALS(PIXEL(255, 255, 255, 255), pixel);

	pixel = pixelmap_get_pixel(&pmap, 1, 1);
	PCUT_ASSERT_INT_EQUALS(PIXEL(255, 255, 255, 255), pixel);

	pixel = pixelmap_get_pixel(&pmap, 1, 0);
	PCUT_ASSERT_INT_EQUALS(PIXEL(0, 0, 0, 0), pixel);

	pixel = pixelmap_get_pixel(&pmap, 0, 1);
	PCUT_ASSERT_INT_EQUALS(PIXEL(0, 0, 0, 0), pixel);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_EXPORT(glyph);
