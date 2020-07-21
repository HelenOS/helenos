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
#include <gfx/font.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(font);

static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);

static gfx_context_ops_t test_ops = {
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect
};

/** Test creating and destroying font */
PCUT_TEST(create_destroy)
{
	gfx_font_metrics_t metrics;
	gfx_font_t *font;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);
	rc = gfx_font_create(gc, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_get_metrics() */
PCUT_TEST(get_metrics)
{
	gfx_font_metrics_t metrics;
	gfx_font_metrics_t gmetrics;
	gfx_font_t *font;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);
	metrics.ascent = 1;
	metrics.descent = 2;
	metrics.leading = 3;

	rc = gfx_font_create(gc, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_get_metrics(font, &gmetrics);
	PCUT_ASSERT_INT_EQUALS(metrics.ascent, gmetrics.ascent);
	PCUT_ASSERT_INT_EQUALS(metrics.descent, gmetrics.descent);
	PCUT_ASSERT_INT_EQUALS(metrics.leading, gmetrics.leading);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_set_metrics() */
PCUT_TEST(set_metrics)
{
	gfx_font_metrics_t metrics1;
	gfx_font_metrics_t metrics2;
	gfx_font_metrics_t gmetrics;
	gfx_font_t *font;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics1);
	metrics1.ascent = 1;
	metrics1.descent = 2;
	metrics1.leading = 3;

	rc = gfx_font_create(gc, &metrics1, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_first_glyph() */
PCUT_TEST(first_glyph)
{
	gfx_font_metrics_t metrics;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_font_create(gc, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	glyph = gfx_font_first_glyph(font);
	PCUT_ASSERT_NULL(glyph);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_font_next_glyph() */
PCUT_TEST(next_glyph)
{
	/* TODO */
}

/** Test gfx_font_search_glyph() */
PCUT_TEST(search_glyph)
{
	gfx_font_metrics_t metrics;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_glyph_t *glyph;
	size_t bytes;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&metrics);

	rc = gfx_font_create(gc, &metrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = gfx_font_search_glyph(font, "Hello", &glyph, &bytes);
	PCUT_ASSERT_ERRNO_VAL(ENOENT, rc);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

static errno_t testgc_set_color(void *arg, gfx_color_t *color)
{
	return EOK;
}

static errno_t testgc_fill_rect(void *arg, gfx_rect_t *rect)
{
	return EOK;
}

PCUT_EXPORT(font);
