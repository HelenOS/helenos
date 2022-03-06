/*
 * Copyright (c) 2022 Jiri Svoboda
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
 * @file Font
 */

#include <adt/list.h>
#include <assert.h>
#include <byteorder.h>
#include <errno.h>
#include <gfx/bitmap.h>
#include <gfx/font.h>
#include <gfx/glyph.h>
#include <mem.h>
#include <stdlib.h>
#include "../private/font.h"
#include "../private/glyph.h"
#include "../private/tpf_file.h"
#include "../private/typeface.h"

/** Initialize font metrics structure.
 *
 * Font metrics structure must always be initialized using this function
 * first.
 *
 * @param metrics Font metrics structure
 */
void gfx_font_metrics_init(gfx_font_metrics_t *metrics)
{
	memset(metrics, 0, sizeof(gfx_font_metrics_t));
}

/** Initialize font properties structure.
 *
 * Font properties structure must always be initialized using this function
 * first.
 *
 * @param props Font properties structure
 */
void gfx_font_props_init(gfx_font_props_t *props)
{
	memset(props, 0, sizeof(gfx_font_props_t));
}

/** Get font properties.
 *
 * @param finfo Font info
 * @param props Place to store font properties
 */
void gfx_font_get_props(gfx_font_info_t *finfo, gfx_font_props_t *props)
{
	*props = finfo->props;
}

/** Create font with existing font info structure.
 *
 * @param tface Typeface
 * @param finfo Font info
 * @param metrics Font metrics
 * @param rfont Place to store pointer to new font
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
static errno_t gfx_font_create_with_info(gfx_typeface_t *tface,
    gfx_font_info_t *finfo, gfx_font_metrics_t *metrics, gfx_font_t **rfont)
{
	gfx_font_t *font = NULL;
	gfx_bitmap_params_t params;
	errno_t rc;

	font = calloc(1, sizeof(gfx_font_t));
	if (font == NULL) {
		rc = ENOMEM;
		goto error;
	}
	font->typeface = tface;
	font->finfo = finfo;

	font->rect.p0.x = 0;
	font->rect.p0.y = 0;
	font->rect.p1.x = 1;
	font->rect.p1.y = 1;

	rc = gfx_font_set_metrics(font, metrics);
	if (rc != EOK) {
		assert(rc == EINVAL);
		goto error;
	}

	/* Create font bitmap */
	gfx_bitmap_params_init(&params);
	params.rect = font->rect;
	params.flags = bmpf_color_key | bmpf_colorize;
	params.key_color = PIXEL(0, 0, 0, 0);

	rc = gfx_bitmap_create(tface->gc, &params, NULL, &font->bitmap);
	if (rc != EOK)
		goto error;

	font->metrics = *metrics;
	list_initialize(&font->glyphs);
	*rfont = font;
	return EOK;
error:
	if (font != NULL)
		free(font);
	return rc;
}

/** Create font.
 *
 * @param tface Typeface
 * @param props Font properties
 * @param metrics Font metrics
 * @param rfont Place to store pointer to new font
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_font_create(gfx_typeface_t *tface, gfx_font_props_t *props,
    gfx_font_metrics_t *metrics, gfx_font_t **rfont)
{
	gfx_font_info_t *finfo = NULL;
	gfx_font_t *font = NULL;
	errno_t rc;

	finfo = calloc(1, sizeof(gfx_font_info_t));
	if (finfo == NULL) {
		rc = ENOMEM;
		goto error;
	}

	finfo->typeface = tface;
	finfo->props = *props;

	rc = gfx_font_create_with_info(tface, finfo, metrics, &font);
	if (rc != EOK)
		goto error;

	finfo->font = font;
	list_append(&finfo->lfonts, &tface->fonts);
	*rfont = font;
	return EOK;
error:
	if (finfo != NULL)
		free(finfo);
	return rc;
}

/** Create dummy font for printing text in text mode.
 *
 * @param tface Typeface
 * @param rfont Place to store pointer to new font
 *
 * @return EOK on success, EINVAL if parameters are invald,
 *         ENOMEM if insufficient resources, EIO if graphic device connection
 *         was lost
 */
errno_t gfx_font_create_textmode(gfx_typeface_t *tface, gfx_font_t **rfont)
{
	gfx_font_props_t props;
	gfx_font_metrics_t metrics;

	gfx_font_props_init(&props);
	props.size = 1;
	props.flags = gff_text_mode;

	gfx_font_metrics_init(&metrics);
	metrics.ascent = 0;
	metrics.descent = 0;
	metrics.leading = 1;

	return gfx_font_create(tface, &props, &metrics, rfont);
}

/** Open font.
 *
 * @param finfo Font info
 * @param rfont Place to store pointer to open font
 * @return EOK on success or an error code
 */
errno_t gfx_font_open(gfx_font_info_t *finfo, gfx_font_t **rfont)
{
	errno_t rc;

	if (finfo->font == NULL) {
		/* Load absent font from TPF file */
		rc = gfx_font_load(finfo);
		if (rc != EOK)
			return rc;

		assert(finfo->font != NULL);
		finfo->font->finfo = finfo;
	}

	*rfont = finfo->font;
	return EOK;
}

/** Close font.
 *
 * @param font Font
 */
void gfx_font_close(gfx_font_t *font)
{
	gfx_glyph_t *glyph;

	glyph = gfx_font_first_glyph(font);
	while (glyph != NULL) {
		gfx_glyph_destroy(glyph);
		glyph = gfx_font_first_glyph(font);
	}

	font->finfo->font = NULL;
	free(font);
}

/** Get font metrics.
 *
 * @param font Font
 * @param metrics Place to store metrics
 */
void gfx_font_get_metrics(gfx_font_t *font, gfx_font_metrics_t *metrics)
{
	*metrics = font->metrics;
}

/** Set font metrics.
 *
 * @param font Font
 * @param metrics Place to store metrics
 * @return EOK on success, EINVAL if supplied metrics are invalid
 */
errno_t gfx_font_set_metrics(gfx_font_t *font, gfx_font_metrics_t *metrics)
{
	font->metrics = *metrics;
	return EOK;
}

/** Get first glyph in font.
 *
 * @param font Font
 * @return First glyph or @c NULL if there are none
 */
gfx_glyph_t *gfx_font_first_glyph(gfx_font_t *font)
{
	link_t *link;

	link = list_first(&font->glyphs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_t, lglyphs);
}

/** Get next glyph in font.
 *
 * @param cur Current glyph
 * @return Next glyph in font or @c NULL if @a cur was the last one
 */
gfx_glyph_t *gfx_font_next_glyph(gfx_glyph_t *cur)
{
	link_t *link;

	link = list_next(&cur->lglyphs, &cur->font->glyphs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_t, lglyphs);
}

/** Get last glyph in font.
 *
 * @param font Font
 * @return Last glyph or @c NULL if there are none
 */
gfx_glyph_t *gfx_font_last_glyph(gfx_font_t *font)
{
	link_t *link;

	link = list_last(&font->glyphs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_t, lglyphs);
}

/** Get previous glyph in font.
 *
 * @param cur Current glyph
 * @return Previous glyph in font or @c NULL if @a cur was the last one
 */
gfx_glyph_t *gfx_font_prev_glyph(gfx_glyph_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lglyphs, &cur->font->glyphs);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, gfx_glyph_t, lglyphs);
}

/** Search for glyph that should be set for the beginning of a string.
 *
 * @param font Font
 * @param str String whose beginning we would like to set
 * @param rglyph Place to store glyph that should be set
 * @param rsize Place to store number of bytes to advance in the string
 * @return EOK on success, ENOENT if no matching glyph was found
 */
errno_t gfx_font_search_glyph(gfx_font_t *font, const char *str,
    gfx_glyph_t **rglyph, size_t *rsize)
{
	gfx_glyph_t *glyph;
	size_t msize;

	glyph = gfx_font_first_glyph(font);
	while (glyph != NULL) {
		if (gfx_glyph_matches(glyph, str, &msize)) {
			*rglyph = glyph;
			*rsize = msize;
			return EOK;
		}

		glyph = gfx_font_next_glyph(glyph);
	}

	return ENOENT;
}

/** Replace glyph graphic with empty space of specified width.
 *
 * This is used to resize a glyph in the font bitmap. This changes
 * the bitmap widht might also make the bitmap taller.
 * Dimensions of the glyph are also adjusted according to @a nrect.
 *
 * @param font Font
 * @param glyph Glyph to replace
 * @param nrect Replacement rectangle
 */
errno_t gfx_font_splice_at_glyph(gfx_font_t *font, gfx_glyph_t *glyph,
    gfx_rect_t *nrect)
{
	gfx_glyph_t *g;
	gfx_bitmap_t *nbitmap;
	gfx_bitmap_params_t params;
	gfx_coord_t dwidth;
	gfx_coord_t x0;
	errno_t rc;

	/* Change of width of glyph */
	dwidth = (nrect->p1.x - nrect->p0.x) -
	    (glyph->rect.p1.x - glyph->rect.p0.x);

	/* Create new font bitmap, wider by dwidth pixels */
	gfx_bitmap_params_init(&params);
	params.rect = font->rect;
	params.rect.p1.x += dwidth;
	if (nrect->p1.y - nrect->p0.y > params.rect.p1.y)
		params.rect.p1.y = nrect->p1.y - nrect->p0.y;
	params.flags = bmpf_color_key | bmpf_colorize;
	params.key_color = PIXEL(0, 0, 0, 0);

	rc = gfx_bitmap_create(font->typeface->gc, &params, NULL, &nbitmap);
	if (rc != EOK)
		goto error;

	/*
	 * In x0 we compute the left margin of @a glyph. We start with
	 * zero and then, if there are any preceding glyphs, we set it
	 * to the right margin of the last one.
	 */
	x0 = 0;

	/* Transfer glyphs before @a glyph */
	g = gfx_font_first_glyph(font);
	while (g != glyph) {
		assert(g != NULL);

		rc = gfx_glyph_transfer(g, 0, nbitmap, &params.rect);
		if (rc != EOK)
			goto error;

		/* Left margin of the next glyph */
		x0 = g->rect.p1.x;

		g = gfx_font_next_glyph(g);
	}

	/* Skip @a glyph */
	g = gfx_font_next_glyph(g);

	/* Transfer glyphs after glyph */
	while (g != NULL) {
		rc = gfx_glyph_transfer(g, dwidth, nbitmap, &params.rect);
		if (rc != EOK)
			goto error;

		/* Update glyph coordinates */
		g->rect.p0.x += dwidth;
		g->rect.p1.x += dwidth;
		g->origin.x += dwidth;

		g = gfx_font_next_glyph(g);
	}

	/* Place glyph rectangle inside the newly created space */
	glyph->origin.x = x0 - nrect->p0.x;
	glyph->origin.y = 0 - nrect->p0.y;
	gfx_rect_translate(&glyph->origin, nrect, &glyph->rect);

	/* Update font bitmap */
	gfx_bitmap_destroy(font->bitmap);
	font->bitmap = nbitmap;
	font->rect = params.rect;

	return EOK;
error:
	if (nbitmap != NULL)
		gfx_bitmap_destroy(nbitmap);
	return rc;
}

/** Load font properties from RIFF TPF file.
 *
 * @param parent Parent chunk
 * @param props Font properties
 * @return EOK on success or an error code
 */
static errno_t gfx_font_props_load(riff_rchunk_t *parent,
    gfx_font_props_t *props)
{
	errno_t rc;
	riff_rchunk_t propsck;
	tpf_font_props_t tprops;
	size_t nread;

	rc = riff_rchunk_match(parent, CKID_fprp, &propsck);
	if (rc != EOK)
		return rc;

	rc = riff_read(&propsck, (void *) &tprops, sizeof(tprops), &nread);
	if (rc != EOK || nread != sizeof(tprops))
		return EIO;

	rc = riff_rchunk_end(&propsck);
	if (rc != EOK)
		return rc;

	props->size = uint16_t_le2host(tprops.size);
	props->flags = uint16_t_le2host(tprops.flags);

	return EOK;
}

/** Save font properties to RIFF TPF file.
 *
 * @param props Font properties
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_font_props_save(gfx_font_props_t *props, riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t propsck;
	tpf_font_props_t tprops;

	tprops.size = host2uint16_t_le(props->size);
	tprops.flags = host2uint16_t_le(props->flags);

	rc = riff_wchunk_start(riffw, CKID_fprp, &propsck);
	if (rc != EOK)
		return rc;

	rc = riff_write(riffw, (void *) &tprops, sizeof(tprops));
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_end(riffw, &propsck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Load font metrics from RIFF TPF file.
 *
 * @param parent Parent chunk
 * @param metrics Font metrics
 * @return EOK on success or an error code
 */
static errno_t gfx_font_metrics_load(riff_rchunk_t *parent,
    gfx_font_metrics_t *metrics)
{
	errno_t rc;
	riff_rchunk_t mtrck;
	tpf_font_metrics_t tmetrics;
	size_t nread;

	rc = riff_rchunk_match(parent, CKID_fmtr, &mtrck);
	if (rc != EOK)
		return rc;

	rc = riff_read(&mtrck, (void *) &tmetrics, sizeof(tmetrics), &nread);
	if (rc != EOK || nread != sizeof(tmetrics))
		return EIO;

	rc = riff_rchunk_end(&mtrck);
	if (rc != EOK)
		return rc;

	metrics->ascent = uint16_t_le2host(tmetrics.ascent);
	metrics->descent = uint16_t_le2host(tmetrics.descent);
	metrics->leading = uint16_t_le2host(tmetrics.leading);
	metrics->underline_y0 = uint16_t_le2host(tmetrics.underline_y0);
	metrics->underline_y1 = uint16_t_le2host(tmetrics.underline_y1);
	return EOK;
}

/** Save font metrics to RIFF TPF file.
 *
 * @param metrics Font metrics
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_font_metrics_save(gfx_font_metrics_t *metrics,
    riffw_t *riffw)
{
	errno_t rc;
	tpf_font_metrics_t tmetrics;
	riff_wchunk_t mtrck;

	tmetrics.ascent = host2uint16_t_le(metrics->ascent);
	tmetrics.descent = host2uint16_t_le(metrics->descent);
	tmetrics.leading = host2uint16_t_le(metrics->leading);
	tmetrics.underline_y0 = host2uint16_t_le(metrics->underline_y0);
	tmetrics.underline_y1 = host2uint16_t_le(metrics->underline_y1);

	rc = riff_wchunk_start(riffw, CKID_fmtr, &mtrck);
	if (rc != EOK)
		return rc;

	rc = riff_write(riffw, (void *) &tmetrics, sizeof(tmetrics));
	if (rc != EOK)
		return rc;

	rc = riff_wchunk_end(riffw, &mtrck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Bit pack font bitmap into 1 bits/pixel format.
 *
 * @param width Bitmap width
 * @param height Bitmap height
 * @param pixels Bitmap pixels
 * @param rdata Place to store pointer to packed data
 * @param rsize Place to store size of packed data
 * @return EOK on sucess, ENOMEM if out of memory
 */
errno_t gfx_font_bitmap_pack(gfx_coord_t width, gfx_coord_t height,
    uint32_t *pixels, void **rdata, size_t *rsize)
{
	void *data;
	size_t size;
	size_t bytes_line;
	gfx_coord_t x, y;
	uint8_t b;
	uint8_t *dp;
	uint32_t *sp;
	uint32_t pix;

	bytes_line = (width + 7) / 8;
	size = height * bytes_line;

	data = malloc(size);
	if (data == NULL)
		return ENOMEM;

	sp = pixels;
	dp = data;
	for (y = 0; y < height; y++) {
		b = 0;
		for (x = 0; x < width; x++) {
			pix = *sp++;
			b = (b << 1) | (pix & 1);
			/* Write eight bits */
			if ((x & 7) == 7) {
				*dp++ = b;
				b = 0;
			}
		}

		/* Write remaining bits */
		if ((x & 7) != 0) {
			b = b << (8 - (x & 7));
			*dp++ = b;
		}
	}

	*rdata = data;
	*rsize = size;
	return EOK;
}

/** Unpack font bitmap from 1 bits/pixel format.
 *
 * @param width Bitmap width
 * @param height Bitmap height
 * @param data Packed data
 * @param dsize Size of packed data
 * @param pixels Bitmap pixels
 * @return EOK on success, EINVAL if data size is invalid
 */
errno_t gfx_font_bitmap_unpack(gfx_coord_t width, gfx_coord_t height,
    void *data, size_t dsize, uint32_t *pixels)
{
	size_t size;
	size_t bytes_line;
	gfx_coord_t x, y;
	uint8_t b;
	uint8_t *sp;
	uint32_t *dp;

	bytes_line = (width + 7) / 8;
	size = height * bytes_line;

	if (dsize != size)
		return EINVAL;

	sp = data;
	dp = pixels;
	for (y = 0; y < height; y++) {
		b = 0;
		for (x = 0; x < width; x++) {
			if ((x & 7) == 0)
				b = *sp++;
			*dp++ = (b >> 7) ? PIXEL(255, 255, 255, 255) :
			    PIXEL(0, 0, 0, 0);
			b = b << 1;
		}
	}

	return EOK;
}

/** Load font bitmap from RIFF TPF file.
 *
 * @param parent Parent chunk
 * @param font Font
 * @return EOK on success or an error code
 */
static errno_t gfx_font_bitmap_load(riff_rchunk_t *parent, gfx_font_t *font)
{
	errno_t rc;
	riff_rchunk_t bmpck;
	tpf_font_bmp_hdr_t thdr;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *bitmap = NULL;
	uint32_t width;
	uint32_t height;
	uint16_t fmt;
	uint16_t depth;
	size_t bytes_line;
	void *data = NULL;
	size_t size;
	size_t nread;

	rc = riff_rchunk_match(parent, CKID_fbmp, &bmpck);
	if (rc != EOK)
		goto error;

	rc = riff_read(&bmpck, &thdr, sizeof(thdr), &nread);
	if (rc != EOK || nread != sizeof(thdr))
		goto error;

	width = uint32_t_le2host(thdr.width);
	height = uint32_t_le2host(thdr.height);
	fmt = uint16_t_le2host(thdr.fmt);
	depth = uint16_t_le2host(thdr.depth);

	if (fmt != 0 || depth != 1) {
		rc = ENOTSUP;
		goto error;
	}

	bytes_line = (width + 7) / 8;
	size = height * bytes_line;

	data = malloc(size);
	if (data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	gfx_bitmap_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = width;
	params.rect.p1.y = height;
	params.flags = bmpf_color_key | bmpf_colorize;
	params.key_color = PIXEL(0, 0, 0, 0);

	rc = gfx_bitmap_create(font->typeface->gc, &params, NULL, &bitmap);
	if (rc != EOK)
		goto error;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		goto error;

	rc = riff_read(&bmpck, data, size, &nread);
	if (rc != EOK)
		goto error;

	if (nread != size) {
		rc = EIO;
		goto error;
	}

	rc = riff_rchunk_end(&bmpck);
	if (rc != EOK)
		goto error;

	rc = gfx_font_bitmap_unpack(width, height, data, size, alloc.pixels);
	if (rc != EOK)
		goto error;

	free(data);
	gfx_bitmap_destroy(font->bitmap);
	font->bitmap = bitmap;
	font->rect = params.rect;
	return EOK;
error:
	if (data != NULL)
		free(data);
	if (bitmap != NULL)
		gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Save font bitmap to RIFF TPF file.
 *
 * @param font Font
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
static errno_t gfx_font_bitmap_save(gfx_font_t *font, riffw_t *riffw)
{
	errno_t rc;
	tpf_font_bmp_hdr_t thdr;
	riff_wchunk_t bmpck;
	gfx_bitmap_alloc_t alloc;
	void *data = NULL;
	size_t dsize;

	rc = gfx_bitmap_get_alloc(font->bitmap, &alloc);
	if (rc != EOK)
		return rc;

	rc = gfx_font_bitmap_pack(font->rect.p1.x, font->rect.p1.y,
	    alloc.pixels, &data, &dsize);
	if (rc != EOK)
		return rc;

	thdr.width = host2uint32_t_le(font->rect.p1.x);
	thdr.height = host2uint32_t_le(font->rect.p1.y);
	thdr.fmt = 0;
	thdr.depth = host2uint16_t_le(1);

	rc = riff_wchunk_start(riffw, CKID_fbmp, &bmpck);
	if (rc != EOK)
		goto error;

	rc = riff_write(riffw, &thdr, sizeof(thdr));
	if (rc != EOK)
		goto error;

	rc = riff_write(riffw, data, dsize);
	if (rc != EOK)
		goto error;

	rc = riff_wchunk_end(riffw, &bmpck);
	if (rc != EOK)
		goto error;

	free(data);
	return EOK;
error:
	if (data != NULL)
		free(data);
	return rc;
}

/** Load font info from RIFF TPF file.
 *
 * @param tface Containing typeface
 * @param parent Parent chunk
 * @return EOK on success or an error code
 */
errno_t gfx_font_info_load(gfx_typeface_t *tface, riff_rchunk_t *parent)
{
	errno_t rc;
	riff_rchunk_t fontck;
	gfx_font_props_t props;
	gfx_font_info_t *finfo = NULL;
	gfx_font_t *font = NULL;

	rc = riff_rchunk_list_match(parent, LTYPE_font, &fontck);
	if (rc != EOK)
		goto error;

	finfo = calloc(1, sizeof(gfx_font_info_t));
	if (finfo == NULL) {
		rc = ENOMEM;
		goto error;
	}

	finfo->fontck = fontck;

	rc = gfx_font_props_load(&fontck, &props);
	if (rc != EOK)
		goto error;

	rc = riff_rchunk_end(&fontck);
	if (rc != EOK)
		goto error;

	finfo->typeface = tface;
	list_append(&finfo->lfonts, &tface->fonts);
	finfo->props = props;
	finfo->font = NULL;

	return EOK;
error:
	if (finfo != NULL)
		free(finfo);
	if (font != NULL)
		gfx_font_close(font);
	return rc;
}

/** Load font from RIFF TPF file.
 *
 * @param finfo Font information
 * @return EOK on success or an error code
 */
errno_t gfx_font_load(gfx_font_info_t *finfo)
{
	errno_t rc;
	gfx_font_metrics_t metrics;
	gfx_font_t *font = NULL;

	/* Seek to beginning of chunk (just after list type) */
	rc = riff_rchunk_seek(&finfo->fontck, sizeof(uint32_t), SEEK_SET);
	if (rc != EOK)
		goto error;

	rc = gfx_font_props_load(&finfo->fontck, &finfo->props);
	if (rc != EOK)
		goto error;

	rc = gfx_font_metrics_load(&finfo->fontck, &metrics);
	if (rc != EOK)
		goto error;

	rc = gfx_font_create_with_info(finfo->typeface, finfo, &metrics, &font);
	if (rc != EOK)
		goto error;

	rc = gfx_font_bitmap_load(&finfo->fontck, font);
	if (rc != EOK)
		goto error;

	while (true) {
		rc = gfx_glyph_load(font, &finfo->fontck);
		if (rc == ENOENT)
			break;
		if (rc != EOK)
			goto error;
	}

	finfo->font = font;
	return EOK;
error:
	if (font != NULL)
		gfx_font_close(font);
	return rc;
}

/** Save font into RIFF TPF file.
 *
 * @param finfo Font info
 * @param riffw RIFF writer
 * @return EOK on success or an error code
 */
errno_t gfx_font_save(gfx_font_info_t *finfo, riffw_t *riffw)
{
	errno_t rc;
	riff_wchunk_t fontck;
	gfx_glyph_t *glyph;

	rc = riff_wchunk_start(riffw, CKID_LIST, &fontck);
	if (rc != EOK)
		return rc;

	rc = riff_write_uint32(riffw, LTYPE_font);
	if (rc != EOK)
		return rc;

	rc = gfx_font_props_save(&finfo->props, riffw);
	if (rc != EOK)
		return rc;

	rc = gfx_font_metrics_save(&finfo->font->metrics, riffw);
	if (rc != EOK)
		return rc;

	rc = gfx_font_bitmap_save(finfo->font, riffw);
	if (rc != EOK)
		return rc;

	glyph = gfx_font_first_glyph(finfo->font);
	while (glyph != NULL) {
		rc = gfx_glyph_save(glyph, riffw);
		if (rc != EOK)
			return rc;

		glyph = gfx_font_next_glyph(glyph);
	}

	rc = riff_wchunk_end(riffw, &fontck);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
