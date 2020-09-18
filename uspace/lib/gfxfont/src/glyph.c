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

/** @addtogroup libgfxfont
 * @{
 */
/**
 * @file Glyph
 */

#include <assert.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/glyph.h>
#include <io/pixelmap.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include "../private/font.h"
#include "../private/glyph.h"
#include "../private/tpf_file.h"

/** Initialize glyph metrics structure.
 *
 * Glyph metrics structure must always be initialized using this function
 * first.
 *
 * @param metrics Glyph metrics structure
 */
void gfx_glyph_metrics_init(gfx_glyph_metrics_t *metrics)
{
	memset(metrics, 0, sizeof(gfx_glyph_metrics_t));
}

/** Create glyph.
 *
 * @param font Containing font
 * @param metrics Glyph metrics
 * @param rglyph Place to store pointer to new glyph
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_glyph_create(gfx_font_t *font, gfx_glyph_metrics_t *metrics,
    gfx_glyph_t **rglyph)
{
	gfx_glyph_t *glyph;
	errno_t rc;

	glyph = calloc(1, sizeof(gfx_glyph_t));
	if (glyph == NULL)
		return ENOMEM;

	glyph->font = font;

	rc = gfx_glyph_set_metrics(glyph, metrics);
	if (rc != EOK) {
		assert(rc == EINVAL);
		free(glyph);
		return rc;
	}

	glyph->metrics = *metrics;
	list_append(&glyph->lglyphs, &glyph->font->glyphs);
	list_initialize(&glyph->patterns);
	*rglyph = glyph;
	return EOK;
}

/** Destroy glyph.
 *
 * @param glyph Glyph
 */
void gfx_glyph_destroy(gfx_glyph_t *glyph)
{
	list_remove(&glyph->lglyphs);
	free(glyph);
}

/** Get glyph metrics.
 *
 * @param glyph Glyph
 * @param metrics Place to store metrics
 */
void gfx_glyph_get_metrics(gfx_glyph_t *glyph, gfx_glyph_metrics_t *metrics)
{
	*metrics = glyph->metrics;
}

/** Set glyph metrics.
 *
 * @param glyph Glyph
 * @param metrics Place to store metrics
 * @return EOK on success, EINVAL if supplied metrics are invalid
 */
errno_t gfx_glyph_set_metrics(gfx_glyph_t *glyph, gfx_glyph_metrics_t *metrics)
{
	glyph->metrics = *metrics;
	return EOK;
}

/** Set a pattern that the glyph will match.
 *
 * A glyph can match any number of patterns. Setting the same pattern
 * again has no effect. The pattern is a simple (sub)string. Matching
 * is done using maximum munch rule.
 *
 * @param glyph Glyph
 * @param pattern Pattern
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_glyph_set_pattern(gfx_glyph_t *glyph, const char *pattern)
{
	gfx_glyph_pattern_t *pat;

	pat = gfx_glyph_first_pattern(glyph);
	while (pat != NULL) {
		if (str_cmp(pat->text, pattern) == 0) {
			/* Already set */
			return EOK;
		}

		pat = gfx_glyph_next_pattern(pat);
	}

	pat = calloc(1, sizeof(gfx_glyph_pattern_t));
	if (pat == NULL)
		return ENOMEM;

	pat->glyph = glyph;
	pat->text = str_dup(pattern);
	if (pat->text == NULL) {
		free(pat);
		return ENOMEM;
	}

	list_append(&pat->lpatterns, &glyph->patterns);
	return EOK;
}

/** Clear a matching pattern from a glyph.
 *
 * Clearing a pattern that is not set has no effect.
 *
 * @param Glyph
 * @param pattern Pattern
 */
void gfx_glyph_clear_pattern(gfx_glyph_t *glyph, const char *pattern)
{
	gfx_glyph_pattern_t *pat;

	pat = gfx_glyph_first_pattern(glyph);
	while (pat != NULL) {
		if (str_cmp(pat->text, pattern) == 0) {
			list_remove(&pat->lpatterns);
			free(pat->text);
			free(pat);
			return;
		}

		pat = gfx_glyph_next_pattern(pat);
	}
}

/** Determine if glyph maches the beginning of a string.
 *
 * @param glyph Glyph
 * @param str String
 * @param rsize Place to store number of bytes in the matching pattern
 * @return @c true iff glyph matches the beginning of the string
 */
bool gfx_glyph_matches(gfx_glyph_t *glyph, const char *str, size_t *rsize)
{
	gfx_glyph_pattern_t *pat;

	pat = gfx_glyph_first_pattern(glyph);
	while (pat != NULL) {
		if (str_test_prefix(str, pat->text)) {
			*rsize = str_size(pat->text);
			return true;
		}

		pat = gfx_glyph_next_pattern(pat);
	}

	return false;
}

/** Get first glyph pattern.
 *
 * @param glyph Glyph
 * @return First pattern or @c NULL if there are none
 */
gfx_glyph_pattern_t *gfx_glyph_first_pattern(gfx_glyph_t *glyph)
{
	link_t *link;

	link = list_first(&glyph->patterns);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_pattern_t, lpatterns);
}

/** Get next glyph pattern.
 *
 * @param cur Current pattern
 * @return Next pattern or @c NULL if there are none
 */
gfx_glyph_pattern_t *gfx_glyph_next_pattern(gfx_glyph_pattern_t *cur)
{
	link_t *link;

	link = list_next(&cur->lpatterns, &cur->glyph->patterns);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_pattern_t, lpatterns);
}

/** Return pattern string.
 *
 * @param pattern Pattern
 * @return Pattern string (owned by @a pattern)
 */
const char *gfx_glyph_pattern_str(gfx_glyph_pattern_t *pattern)
{
	return pattern->text;
}

/** Render glyph to GC.
 *
 * @param glyph Glyph
 * @param pos Position to render to (where glyph origin is placed)
 */
errno_t gfx_glyph_render(gfx_glyph_t *glyph, gfx_coord2_t *pos)
{
	gfx_coord2_t offs;

	gfx_coord2_subtract(pos, &glyph->origin, &offs);

	return gfx_bitmap_render(glyph->font->bitmap, &glyph->rect, &offs);
}

/** Transfer glyph to new font bitmap.
 *
 * @param glyph Glyph
 * @param offs Offset in new font bitmap
 * @param dest New font bitmap
 * @param drect Bounding rectangle for @a dest
 *
 * @return EOK on success or an error code
 */
errno_t gfx_glyph_transfer(gfx_glyph_t *glyph, gfx_coord_t offs,
    gfx_bitmap_t *dest, gfx_rect_t *drect)
{
	pixelmap_t smap;
	pixelmap_t dmap;
	gfx_bitmap_alloc_t salloc;
	gfx_bitmap_alloc_t dalloc;
	gfx_coord_t x, y;
	pixel_t pixel;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(glyph->font->bitmap, &salloc);
	if (rc != EOK)
		return rc;

	rc = gfx_bitmap_get_alloc(dest, &dalloc);
	if (rc != EOK)
		return rc;

	smap.width = glyph->font->rect.p1.x;
	smap.height = glyph->font->rect.p1.y;
	smap.data = salloc.pixels;

	dmap.width = drect->p1.x;
	dmap.height = drect->p1.y;
	dmap.data = dalloc.pixels;

	for (y = drect->p0.y; y < drect->p1.y; y++) {
		for (x = drect->p0.x; x < drect->p1.x; x++) {
			pixel = pixelmap_get_pixel(&smap, x, y);
			pixelmap_put_pixel(&dmap, x + offs, y, pixel);
		}
	}

	return EOK;
}

/** Save glyph metrics to RIFF TPF file.
 *
 * @param metrics Glyph metrics
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_glyph_metrics_save(gfx_glyph_metrics_t *metrics,
    riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t mtrck;

	rc = riff_wchunk_start(riffw, CKID_gmtr, &mtrck);
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_write(riffw, (void *) metrics, sizeof(*metrics));
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_end(riffw, &mtrck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Save glyph patterns to RIFF TPF file.
 *
 * @param glyph Glyph
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_glyph_patterns_save(gfx_glyph_t *glyph, riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t patck;
	gfx_glyph_pattern_t *pat;
	const char *str;

	rc = riff_wchunk_start(riffw, CKID_gpat, &patck);
	if (rc != EOK)
		return rc;

	pat = gfx_glyph_first_pattern(glyph);
	while (pat != NULL) {
		str = gfx_glyph_pattern_str(pat);

		rc = riff_wchunk_write(riffw, (void *) str, 1 + str_size(str));
		if (rc != EOK)
			return rc;

		pat = gfx_glyph_next_pattern(pat);
	}

	rc = riff_wchunk_end(riffw, &patck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Save glyph rectangle/origin to RIFF TPF file.
 *
 * @param glyph Glyph
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_glyph_rectangle_origin_save(gfx_glyph_t *glyph,
    riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t rorck;

	rc = riff_wchunk_start(riffw, CKID_gror, &rorck);
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_write(riffw, (void *) &glyph->rect,
	    sizeof(glyph->rect));
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_write(riffw, (void *) &glyph->origin,
	    sizeof(glyph->origin));
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_end(riffw, &rorck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Save glyph into RIFF TPF file.
 *
 * @param glyph Glyph
 * @param riffw RIFF writer
 */
errno_t gfx_glyph_save(gfx_glyph_t *glyph, riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t glyphck;

	rc = riff_wchunk_start(riffw, CKID_LIST, &glyphck);
	if (rc != EOK)
		return rc;

	rc = riff_write_uint32(riffw, LTYPE_glph);
	if (rc != EOK)
		return rc;

	rc = gfx_glyph_metrics_save(&glyph->metrics, riffw);
	if (rc != EOK)
		return rc;

	rc = gfx_glyph_patterns_save(glyph, riffw);
	if (rc != EOK)
		return rc;

	rc = gfx_glyph_rectangle_origin_save(glyph, riffw);
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_end(riffw, &glyphck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
