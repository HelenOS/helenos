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
#include "../private/glyph_bmp.h"
#include "../private/testgc.h"

PCUT_INIT;

PCUT_TEST_SUITE(glyph_bmp);

/** Test opening and closing glyph bitmap */
PCUT_TEST(open_close)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_save() */
PCUT_TEST(save)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	int pix;
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

	bmp = NULL;

	/* Open bitmap and set some pixels */

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
	bmp = NULL;

	/* Re-open the saved bimap and verify pixel values were preserved */

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 1, 1);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 1, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 1);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	/* ... */

	rc = gfx_glyph_bmp_setpix(bmp, 1, -1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_save(bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_bmp_close(bmp);

	/* Once again */

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 1, 1);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 1, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 1);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 1, -1);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	pix = gfx_glyph_bmp_getpix(bmp, 0, -1);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_getpix() */
PCUT_TEST(getpix)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	test_gc_t tgc;
	int pix;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_setpix() can flip pixel value both ways */
PCUT_TEST(setpix_flip)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	test_gc_t tgc;
	int pix;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(1, pix);

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_setpix() properly extends pixel array */
PCUT_TEST(setpix_extend)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	gfx_coord_t x, y;
	test_gc_t tgc;
	int pix;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	/*
	 * Fill the rectangle [0, 0]..[3, 3] with alternating pixel pattern
	 * and then check it.
	 */

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 1, 1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 2, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 0, 2, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 2, 2, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	for (y = 0; y <= 2; y++) {
		for (x = 0; x <= 2; x++) {
			pix = gfx_glyph_bmp_getpix(bmp, x, y);
			PCUT_ASSERT_INT_EQUALS((x & 1) ^ (y & 1) ^ 1, pix);
		}
	}

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_clear() properly clears bitmap */
PCUT_TEST(clear)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	gfx_rect_t rect;
	test_gc_t tgc;
	int pix;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	/* Set some pixels */

	rc = gfx_glyph_bmp_setpix(bmp, 0, 0, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 1, 1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Clear the bitmap and check */
	gfx_glyph_bmp_clear(bmp);

	gfx_glyph_bmp_get_rect(bmp, &rect);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.y);

	pix = gfx_glyph_bmp_getpix(bmp, 0, 0);
	PCUT_ASSERT_INT_EQUALS(0, pix);
	pix = gfx_glyph_bmp_getpix(bmp, 1, 1);
	PCUT_ASSERT_INT_EQUALS(0, pix);

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test glyph_bmp_find_used_rect() find minimum used rectangle */
PCUT_TEST(find_used_rect)
{
	gfx_font_props_t fprops;
	gfx_font_metrics_t fmetrics;
	gfx_typeface_t *tface;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_bmp_t *bmp;
	gfx_rect_t rect;
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

	bmp = NULL;

	rc = gfx_glyph_bmp_open(glyph, &bmp);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(bmp);

	/* Check used rectangle */

	gfx_glyph_bmp_find_used_rect(bmp, &rect);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(0, rect.p1.y);

	/* Set some pixels */

	rc = gfx_glyph_bmp_setpix(bmp, -4, -5, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, -2, -1, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 3, 4, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 7, 6, 1);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check used rectangle */

	gfx_glyph_bmp_find_used_rect(bmp, &rect);
	PCUT_ASSERT_INT_EQUALS(-4, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(-5, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(8, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(7, rect.p1.y);

	/* Clear the corner pixels */

	rc = gfx_glyph_bmp_setpix(bmp, -4, -5, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_glyph_bmp_setpix(bmp, 7, 6, 0);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Check used rectangle got smaller */

	gfx_glyph_bmp_find_used_rect(bmp, &rect);
	PCUT_ASSERT_INT_EQUALS(-2, rect.p0.x);
	PCUT_ASSERT_INT_EQUALS(-1, rect.p0.y);
	PCUT_ASSERT_INT_EQUALS(4, rect.p1.x);
	PCUT_ASSERT_INT_EQUALS(5, rect.p1.y);

	gfx_glyph_bmp_close(bmp);

	gfx_glyph_destroy(glyph);

	gfx_font_close(font);
	gfx_typeface_destroy(tface);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_EXPORT(glyph_bmp);
