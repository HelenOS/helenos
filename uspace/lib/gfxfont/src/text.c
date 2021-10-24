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
	uint8_t attr;
	pixelmap_t pmap;
	gfx_coord_t x;
	pixel_t pixel;
	char32_t c;
	size_t off;
	errno_t rc;

	/*
	 * NOTE: Creating and destroying bitmap each time is not probably
	 * the most efficient way.
	 */

	gfx_color_get_ega(color, &attr);

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

	off = 0;
	for (x = 0; x < params.rect.p1.x; x++) {
		c = str_decode(str, &off, STR_NO_LIMIT);
		pixel = PIXEL(attr,
		    (c >> 16) & 0xff,
		    (c >> 8) & 0xff,
		    c & 0xff);
		pixelmap_put_pixel(&pmap, x, 0, pixel);
	}

	rc = gfx_bitmap_render(bitmap, NULL, pos);

	gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Get text starting position.
 *
 * @param font Font
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String
 * @param spos Place to store starting position
 */
void gfx_text_start_pos(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_text_fmt_t *fmt, const char *str, gfx_coord2_t *spos)
{
	gfx_font_metrics_t fmetrics;
	gfx_coord_t width;

	*spos = *pos;

	/* Adjust position for horizontal alignment */
	if (fmt->halign != gfx_halign_left) {
		width = gfx_text_width(font, str);
		switch (fmt->halign) {
		case gfx_halign_center:
			spos->x -= width / 2;
			break;
		case gfx_halign_right:
			spos->x -= width;
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
			spos->y += fmetrics.ascent;
			break;
		case gfx_valign_center:
			spos->y += fmetrics.ascent / 2;
			break;
		case gfx_valign_bottom:
			spos->y -= fmetrics.descent + 1;
			break;
		default:
			break;
		}
	}
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
	gfx_glyph_metrics_t gmetrics;
	size_t stradv;
	const char *cp;
	gfx_glyph_t *glyph;
	gfx_coord2_t cpos;
	errno_t rc;

	gfx_text_start_pos(font, pos, fmt, str, &cpos);

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

/** Find character position in string by X coordinate.
 *
 * @param font Font
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String
 * @param fpos Position for which we need to find offset
 *
 * @return Byte offset in @a str of character corresponding to position
 *         @a fpos. Note that the position is rounded, that is,
 *         if it is before the center of character A, it will return
 *         offset of A, if it is after the center of A, it will return
 *         offset of the following character.
 */
size_t gfx_text_find_pos(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_text_fmt_t *fmt, const char *str, gfx_coord2_t *fpos)
{
	gfx_glyph_metrics_t gmetrics;
	size_t stradv;
	const char *cp;
	gfx_glyph_t *glyph;
	gfx_coord2_t cpos;
	size_t off;
	size_t strsize;
	errno_t rc;

	gfx_text_start_pos(font, pos, fmt, str, &cpos);

	/* Text mode */
	if ((font->finfo->props.flags & gff_text_mode) != 0) {
		off = 0;
		strsize = str_size(str);
		while (off < strsize) {
			if (fpos->x <= cpos.x)
				return off;
			(void) str_decode(str, &off, strsize);
			cpos.x++;
		}

		return off;
	}

	cp = str;
	off = 0;
	while (*cp != '\0') {
		rc = gfx_font_search_glyph(font, cp, &glyph, &stradv);
		if (rc != EOK) {
			++cp;
			continue;
		}

		gfx_glyph_get_metrics(glyph, &gmetrics);

		if (fpos->x < cpos.x + gmetrics.advance / 2)
			return off;

		cp += stradv;
		off += stradv;
		cpos.x += gmetrics.advance;
	}

	return off;
}

/** Get text continuation parameters.
 *
 * Return the anchor position and format needed to continue printing
 * text after the specified string. It is allowed for the sources
 * (@a pos, @a fmt) and destinations (@a cpos, @a cfmt) to point
 * to the same objects, respectively.
 *
 * @param font Font
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String
 * @param cpos Place to store anchor position for continuation
 * @param cfmt Place to store format for continuation
 */
void gfx_text_cont(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_text_fmt_t *fmt, const char *str, gfx_coord2_t *cpos,
    gfx_text_fmt_t *cfmt)
{
	gfx_coord2_t spos;
	gfx_text_fmt_t tfmt;

	/* Continuation should start where the current string ends */
	gfx_text_start_pos(font, pos, fmt, str, &spos);
	cpos->x = spos.x + gfx_text_width(font, str);
	cpos->y = spos.y;

	/*
	 * Formatting is the same, except the text should be aligned
	 * so that it starts at the anchor point.
	 */
	tfmt = *fmt;
	tfmt.halign = gfx_halign_left;
	tfmt.valign = gfx_valign_baseline;

	*cfmt = tfmt;
}

/** Get text bounding rectangle.
 *
 * @param font Font
 * @param pos Anchor position
 * @param fmt Text formatting
 * @param str String
 * @param rect Place to store bounding rectangle
 */
void gfx_text_rect(gfx_font_t *font, gfx_coord2_t *pos,
    gfx_text_fmt_t *fmt, const char *str, gfx_rect_t *rect)
{
	gfx_coord2_t spos;

	gfx_text_start_pos(font, pos, fmt, str, &spos);

	rect->p0.x = spos.x;
	rect->p0.y = spos.y - font->metrics.ascent;
	rect->p1.x = spos.x + gfx_text_width(font, str);
	rect->p1.y = spos.y + font->metrics.descent + 1;
}

/** @}
 */
