/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2008 Martin Decky
 * Copyright (c) 2006 Jakub Vana
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup output
 * @{
 */

#include <ddev.h>
#include <errno.h>
#include <fbfont/font-8x16.h>
#include <gfx/bitmap.h>
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/context.h>
#include <gfx/render.h>
#include <io/chargrid.h>
#include <io/pixelmap.h>
#include <stdlib.h>
#include "../output.h"
#include "ddev.h"

#define KFB_DDEV_NAME "devices/\\virt\\kfb\\kfb"

typedef struct {
	gfx_context_t *gc;

	sysarg_t cols;
	sysarg_t rows;

	uint8_t style_normal;
	uint8_t style_inverted;

	gfx_bitmap_t *bitmap;
	pixelmap_t pixelmap;
	gfx_rect_t dirty;

	sysarg_t curs_col;
	sysarg_t curs_row;
	bool curs_visible;
} output_ddev_t;

static pixel_t color_table[16] = {
	[COLOR_BLACK]       = 0x000000,
	[COLOR_BLUE]        = 0x0000f0,
	[COLOR_GREEN]       = 0x00f000,
	[COLOR_CYAN]        = 0x00f0f0,
	[COLOR_RED]         = 0xf00000,
	[COLOR_MAGENTA]     = 0xf000f0,
	[COLOR_YELLOW]      = 0xf0f000,
	[COLOR_WHITE]       = 0xf0f0f0,

	[COLOR_BLACK + 8]   = 0x000000,
	[COLOR_BLUE + 8]    = 0x0000ff,
	[COLOR_GREEN + 8]   = 0x00ff00,
	[COLOR_CYAN + 8]    = 0x00ffff,
	[COLOR_RED + 8]     = 0xff0000,
	[COLOR_MAGENTA + 8] = 0xff00ff,
	[COLOR_YELLOW + 8]  = 0xffff00,
	[COLOR_WHITE + 8]   = 0xffffff,
};

static void attrs_rgb(char_attrs_t attrs, pixel_t *bgcolor, pixel_t *fgcolor)
{
	switch (attrs.type) {
	case CHAR_ATTR_STYLE:
		switch (attrs.val.style) {
		case STYLE_NORMAL:
			*bgcolor = color_table[COLOR_WHITE];
			*fgcolor = color_table[COLOR_BLACK];
			break;
		case STYLE_EMPHASIS:
			*bgcolor = color_table[COLOR_WHITE];
			*fgcolor = color_table[COLOR_RED];
			break;
		case STYLE_INVERTED:
			*bgcolor = color_table[COLOR_BLACK];
			*fgcolor = color_table[COLOR_WHITE];
			break;
		case STYLE_SELECTED:
			*bgcolor = color_table[COLOR_RED];
			*fgcolor = color_table[COLOR_WHITE];
			break;
		}
		break;
	case CHAR_ATTR_INDEX:
		*bgcolor = color_table[(attrs.val.index.bgcolor & 7) |
		    ((attrs.val.index.attr & CATTR_BRIGHT) ? 8 : 0)];
		*fgcolor = color_table[(attrs.val.index.fgcolor & 7) |
		    ((attrs.val.index.attr & CATTR_BRIGHT) ? 8 : 0)];
		break;
	case CHAR_ATTR_RGB:
		*bgcolor = attrs.val.rgb.bgcolor;
		*fgcolor = attrs.val.rgb.fgcolor;
		break;
	}
}

/** Draw the character at the specified position.
 *
 * @param field Character field.
 * @param col   Horizontal screen position.
 * @param row   Vertical screen position.
 *
 */
static void draw_char(output_ddev_t *ddev, charfield_t *field,
    sysarg_t col, sysarg_t row)
{
	pixel_t fgcolor;
	pixel_t bgcolor;
	pixel_t pcolor;
	pixel_t tmp;
	gfx_rect_t rect;
	gfx_rect_t ndrect;
	gfx_coord_t x, y;
	bool inverted;
	uint32_t glyph;

	fgcolor = bgcolor = 0;
	attrs_rgb(field->attrs, &bgcolor, &fgcolor);

	inverted = (col == ddev->curs_col) && (row == ddev->curs_row) &&
	    ddev->curs_visible;

	if (inverted) {
		/* Swap foreground and background colors */
		tmp = bgcolor;
		bgcolor = fgcolor;
		fgcolor = tmp;
	}

	rect.p0.x = FONT_WIDTH * col;
	rect.p0.y = FONT_SCANLINES * row;
	rect.p1.x = FONT_WIDTH * (col + 1);
	rect.p1.y = FONT_SCANLINES * (row + 1);

	glyph = fb_font_glyph(field->ch, NULL);

	for (y = 0; y < FONT_SCANLINES; y++) {
		for (x = 0; x < FONT_WIDTH; x++) {
			pcolor = fb_font[glyph][y] & (1 << (7 - x)) ?
			    fgcolor : bgcolor;
			pixelmap_put_pixel(&ddev->pixelmap,
			    rect.p0.x + x, rect.p0.y + y, pcolor);
		}
	}

	/* Compute new dirty rectangle */
	gfx_rect_envelope(&ddev->dirty, &rect, &ndrect);
	ddev->dirty = ndrect;
}

static errno_t output_ddev_yield(outdev_t *dev)
{
	return EOK;
}

static errno_t output_ddev_claim(outdev_t *dev)
{
	return EOK;
}

static void output_ddev_get_dimensions(outdev_t *dev, sysarg_t *cols, sysarg_t *rows)
{
	output_ddev_t *ddev = (output_ddev_t *) dev->data;

	*cols = ddev->cols;
	*rows = ddev->rows;
}

static console_caps_t output_ddev_get_caps(outdev_t *dev)
{
	return (CONSOLE_CAP_CURSORCTL | CONSOLE_CAP_STYLE |
	    CONSOLE_CAP_INDEXED | CONSOLE_CAP_RGB);
}

static void output_ddev_char_update(outdev_t *dev, sysarg_t col, sysarg_t row)
{
	output_ddev_t *ddev = (output_ddev_t *) dev->data;

	charfield_t *field =
	    chargrid_charfield_at(dev->backbuf, col, row);

	draw_char(ddev, field, col, row);
}

static void output_ddev_cursor_update(outdev_t *dev, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	output_ddev_t *ddev = (output_ddev_t *) dev->data;

	ddev->curs_col = col;
	ddev->curs_row = row;
	ddev->curs_visible = visible;

	output_ddev_char_update(dev, prev_col, prev_row);
	output_ddev_char_update(dev, col, row);
}

static void output_ddev_flush(outdev_t *dev)
{
	output_ddev_t *ddev = (output_ddev_t *) dev->data;

	(void) gfx_bitmap_render(ddev->bitmap, &ddev->dirty, NULL);

	ddev->dirty.p0.x = 0;
	ddev->dirty.p0.y = 0;
	ddev->dirty.p1.x = 0;
	ddev->dirty.p1.y = 0;
}

static outdev_ops_t output_ddev_ops = {
	.yield = output_ddev_yield,
	.claim = output_ddev_claim,
	.get_dimensions = output_ddev_get_dimensions,
	.get_caps = output_ddev_get_caps,
	.cursor_update = output_ddev_cursor_update,
	.char_update = output_ddev_char_update,
	.flush = output_ddev_flush
};

errno_t output_ddev_init(void)
{
	ddev_t *dd = NULL;
	output_ddev_t *ddev = NULL;
	ddev_info_t info;
	gfx_coord2_t dims;
	gfx_context_t *gc;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *bitmap = NULL;
	errno_t rc;

	ddev = calloc(1, sizeof(output_ddev_t));
	if (ddev == NULL)
		return ENOMEM;

	rc = ddev_open(KFB_DDEV_NAME, &dd);
	if (rc != EOK) {
		printf("Error opening display device '%s'.", KFB_DDEV_NAME);
		goto error;
	}

	rc = ddev_get_info(dd, &info);
	if (rc != EOK) {
		printf("Error getting information for display device '%s'.",
		    KFB_DDEV_NAME);
		goto error;
	}

	rc = ddev_get_gc(dd, &gc);
	if (rc != EOK) {
		printf("Error getting device context for '%s'.", KFB_DDEV_NAME);
		goto error;
	}

	gfx_bitmap_params_init(&params);
	params.rect = info.rect;

	rc = gfx_bitmap_create(gc, &params, NULL, &bitmap);
	if (rc != EOK) {
		printf("Error creating screen bitmap.\n");
		goto error;
	}

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK) {
		printf("Error getting bitmap allocation info.\n");
		goto error;
	}

	gfx_rect_dims(&info.rect, &dims);

	ddev->gc = gc;
	ddev->cols = dims.x / FONT_WIDTH;
	ddev->rows = dims.y / FONT_SCANLINES;
	ddev->bitmap = bitmap;
	ddev->pixelmap.width = dims.x;
	ddev->pixelmap.height = dims.y;
	ddev->pixelmap.data = alloc.pixels;

	outdev_t *dev = outdev_register(&output_ddev_ops, (void *) ddev);
	if (dev == NULL) {
		rc = EINVAL;
		goto error;
	}

	return EOK;
error:
	if (dd != NULL)
		ddev_close(dd);
	if (bitmap != NULL)
		gfx_bitmap_destroy(bitmap);
	free(ddev);
	return rc;
}

/** @}
 */
