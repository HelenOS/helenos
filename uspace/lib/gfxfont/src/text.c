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
 * @file Text rendering
 */

#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/pixelmap.h>
#include <mem.h>
#include <str.h>
#include "../private/font.h"
#include "../private/typeface.h"

/** Initialize text formatting structure.
 *
 * Text formatting structure must always be initialized using this function
 * first.
 *
 * @param fmt Text formatting structure
 */
void gfx_text_fmt_init(gfx_text_fmt_t *fmt)
{
	memset(fmt, 0, sizeof(gfx_text_fmt_t));
}

/** Compute text width.
 *
 * @param font Font
 * @param str String
 * @return Text width
 */
gfx_coord_t gfx_text_width(gfx_font_t *font, const char *str)
{
	gfx_glyph_metrics_t gmetrics;
	size_t stradv;
	const char *cp;
	gfx_glyph_t *glyph;
	gfx_coord_t width;
	errno_t rc;

	if ((font->finfo->props.flags & gff_text_mode) != 0)
		return str_width(str);

	width = 0;
	cp = str;
	while (*cp != '\0') {
		rc = gfx_font_search_glyph(font, cp, &glyph, &stradv);
		if (rc != EOK) {
			++cp;
			continue;
		}

		gfx_glyph_get_metrics(glyph, &gmetrics);

		cp += stradv;
		width += gmetrics.advance;
	}

	return width;
}

/** Print string using text characters in text mode.
 *
 * @param font Font
 * @param pos Position of top-left corner of text
 * @param color Text color
 * @param str String
 * @return EOK on success or an error code
 */
static errno_t gfx_puttext_textmode(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_color_t *color, const char *str)
{
	gfx_context_t *gc = font->typeface->gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_t *bitmap;
	gfx_bitmap_alloc_t alloc;
	uint16_t r, g, b;
	pixelmap_t pmap;
	gfx_coord_t x;
	pixel_t pixel;
	errno_t rc;

	/*
	 * NOTE: Creating and destroying bitmap each time is not probably
	 * the most efficient way.
	 */

	gfx_color_get_rgb_i16(color, &r, &g, &b);

	/*
	 * We are setting the *background* color, the foreground color
	 * will be set to its complement.
	 */
	r = 0xff ^ (r >> 8);
	g = 0xff ^ (g >> 8);
	b = 0xff ^ (b >> 8);

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = str_width(str);
	params.rect.p1.y = 1;

	if (params.rect.p1.x == 0) {
		/* Nothing to do. Avoid creating bitmap of zero width. */
		return EOK;
	}

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK) {
		gfx_bitmap_destroy(bitmap);
		return rc;
	}

	pmap.width = params.rect.p1.x;
	pmap.height = 1;
	pmap.data = alloc.pixels;

	for (x = 0; x < params.rect.p1.x; x++) {
		pixel = PIXEL(str[x], r, g, b);
		pixelmap_put_pixel(&pmap, x, 0, pixel);
	}

	rc = gfx_bitmap_render(bitmap, NULL, pos);

	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Render text.
 *
 * @param font Font
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String
 * @return EOK on success or an error code
 */
errno_t gfx_puttext(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_text_fmt_t *fmt, const char *str)
{
	gfx_font_metrics_t fmetrics;
	gfx_glyph_metrics_t gmetrics;
	size_t stradv;
	const char *cp;
	gfx_glyph_t *glyph;
	gfx_coord2_t cpos;
	gfx_coord_t width;
	errno_t rc;

	cpos = *pos;

	/* Adjust position for horizontal alignment */
	if (fmt->halign != gfx_halign_left) {
		width = gfx_text_width(font, str);
		switch (fmt->halign) {
		case gfx_halign_center:
			cpos.x -= width / 2;
			break;
		case gfx_halign_right:
			cpos.x -= width - 1;
			break;
		default:
			break;
		}
	}

	/* Adjust position for vertical alignment */
	gfx_font_get_metrics(font, &fmetrics);

	if (fmt->valign != gfx_valign_baseline) {
		switch (fmt->valign) {
		case gfx_valign_top:
			cpos.y += fmetrics.ascent;
			break;
		case gfx_valign_center:
			cpos.y += fmetrics.ascent / 2;
			break;
		case gfx_valign_bottom:
			cpos.y -= fmetrics.descent;
			break;
		default:
			break;
		}
	}

	/* Text mode */
	if ((font->finfo->props.flags & gff_text_mode) != 0)
		return gfx_puttext_textmode(font, &cpos, fmt->color, str);

	rc = gfx_set_color(font->typeface->gc, fmt->color);
	if (rc != EOK)
		return rc;

	cp = str;
	while (*cp != '\0') {
		rc = gfx_font_search_glyph(font, cp, &glyph, &stradv);
		if (rc != EOK) {
			++cp;
			continue;
		}

		gfx_glyph_get_metrics(glyph, &gmetrics);

		rc = gfx_glyph_render(glyph, &cpos);
		if (rc != EOK)
			return rc;

		cp += stradv;
		cpos.x += gmetrics.advance;
	}

	return EOK;
}

/** @}
 */
