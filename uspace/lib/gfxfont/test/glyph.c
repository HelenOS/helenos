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
#include <gfx/glyph.h>
#include <pcut/pcut.h>
#include <stdbool.h>
#include <str.h>

PCUT_INIT;

PCUT_TEST_SUITE(glyph);

static errno_t testgc_set_color(void *, gfx_color_t *);
static errno_t testgc_fill_rect(void *, gfx_rect_t *);

static gfx_context_ops_t test_ops = {
	.set_color = testgc_set_color,
	.fill_rect = testgc_fill_rect
};

/** Test creating and destroying glyph */
PCUT_TEST(create_destroy)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_destroy(glyph);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_get_metrics() */
PCUT_TEST(get_metrics)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_glyph_metrics_t rmetrics;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_metrics_init(&gmetrics);
	gmetrics.advance = 42;

	rc = gfx_glyph_create(font, &gmetrics, &glyph);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_glyph_get_metrics(glyph, &rmetrics);
	PCUT_ASSERT_INT_EQUALS(gmetrics.advance, rmetrics.advance);

	gfx_glyph_destroy(glyph);

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_set_metrics() */
PCUT_TEST(set_metrics)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics1;
	gfx_glyph_metrics_t gmetrics2;
	gfx_glyph_t *glyph;
	gfx_glyph_metrics_t rmetrics;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_set_pattern() */
PCUT_TEST(set_pattern)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_clear_pattern() */
PCUT_TEST(clear_pattern)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_matches() */
PCUT_TEST(matches)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	bool match;
	size_t msize;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_first_pattern(), gfx_glyph_next_pattern() */
PCUT_TEST(first_next_pattern)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_pattern_t *pat;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

	gfx_font_destroy(font);
	rc = gfx_context_delete(gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

/** Test gfx_glyph_pattern_str() */
PCUT_TEST(pattern_str)
{
	gfx_font_metrics_t fmetrics;
	gfx_font_t *font;
	gfx_glyph_metrics_t gmetrics;
	gfx_glyph_t *glyph;
	gfx_context_t *gc;
	gfx_glyph_pattern_t *pat;
	const char *pstr;
	errno_t rc;

	rc = gfx_context_new(&test_ops, NULL, &gc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	gfx_font_metrics_init(&fmetrics);
	rc = gfx_font_create(gc, &fmetrics, &font);
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

PCUT_EXPORT(glyph);
