/*
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

/** @file
 */

#include <abi/fb/visuals.h>
#include <sys/types.h>
#include <inttypes.h>
#include <screenbuffer.h>
#include <imgmap.h>
#include <errno.h>
#include <bool.h>
#include <sysinfo.h>
#include <ddi.h>
#include <stdlib.h>
#include <mem.h>
#include <as.h>
#include <align.h>
#include "../gfx/font-8x16.h"
#include "../fb.h"
#include "kfb.h"

#define DEFAULT_BGCOLOR  0xffffff
#define DEFAULT_FGCOLOR  0x000000

#define FB_POS(x, y)  ((y) * kfb.scanline + (x) * kfb.pixel_bytes)

#define COL2X(col)  ((col) * FONT_WIDTH)
#define ROW2Y(row)  ((row) * FONT_SCANLINES)
#define X2COL(x)    ((x) / FONT_WIDTH)
#define Y2ROW(y)    ((y) / FONT_SCANLINES)

#define GLYPH_POS(glyph, y, inverted) \
	(((glyph) + (inverted) * FONT_GLYPHS) * (kfb.glyph_bytes) + (y) * (kfb.glyph_scanline))

#define POINTER_WIDTH   11
#define POINTER_HEIGHT  18

static uint8_t pointer[] = {
	0x01, 0x00, 0x03, 0x00, 0x05, 0x00, 0x09, 0x00, 0x11, 0x00, 0x21, 0x00,
	0x41, 0x00, 0x81, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x04, 0x01, 0x03,
	0x81, 0x00, 0x89, 0x00, 0x15, 0x01, 0x23, 0x01, 0x21, 0x01, 0xc0, 0x00
};

static uint8_t pointer_mask[] = {
	0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f, 0x00, 0x3f, 0x00,
	0x7f, 0x00, 0xff, 0x00, 0xff, 0x01, 0xff, 0x03, 0xff, 0x07, 0xff, 0x03,
	0xff, 0x00, 0xff, 0x00, 0xf7, 0x01, 0xe3, 0x01, 0xe1, 0x01, 0xc0, 0x00
};

/** Function to draw a character. */
typedef void (*draw_char_t)(sysarg_t, sysarg_t, bool, wchar_t, pixel_t,
    pixel_t);

typedef struct {
	sysarg_t width;
	sysarg_t height;
	size_t offset;
	size_t scanline;
	visual_t visual;
	
	size_t size;
	uint8_t *addr;
	
	pixel2visual_t pixel2visual;
	visual2pixel_t visual2pixel;
	visual_mask_t visual_mask;
	size_t pixel_bytes;
	
	sysarg_t pointer_x;
	sysarg_t pointer_y;
	bool pointer_visible;
	imgmap_t *pointer_imgmap;
	
	/*
	 * Pre-rendered mask for rendering
	 * glyphs. Specific for the framebuffer
	 * visual.
	 */
	
	size_t glyph_scanline;
	size_t glyph_bytes;
	uint8_t *glyphs;
	
	uint8_t *backbuf;
} kfb_t;

typedef struct {
	/**
	 * Character drawing function for this viewport.
	 * Different viewports might use different drawing
	 * functions depending on whether their scanlines
	 * are aligned on a word boundary.
	 */
	draw_char_t draw_char;
} kfb_vp_t;

static kfb_t kfb;

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

static void put_pixel(sysarg_t x, sysarg_t y, pixel_t pixel)
{
	if ((x >= kfb.width) || (y >= kfb.height))
		return;
	
	kfb.pixel2visual(kfb.addr + FB_POS(x, y), pixel);
}

static pixel_t get_pixel(sysarg_t x, sysarg_t y)
{
	if ((x >= kfb.width) || (y >= kfb.height))
		return 0;
	
	return kfb.visual2pixel(kfb.addr + FB_POS(x, y));
}

static void pointer_show(void)
{
	if (kfb.pointer_visible) {
		for (sysarg_t y = 0; y < POINTER_HEIGHT; y++) {
			for (sysarg_t x = 0; x < POINTER_WIDTH; x++) {
				sysarg_t dx = kfb.pointer_x + x;
				sysarg_t dy = kfb.pointer_y + y;
				
				pixel_t pixel = get_pixel(dx, dy);
				imgmap_put_pixel(kfb.pointer_imgmap, x, y, pixel);
				
				size_t offset = y * ((POINTER_WIDTH - 1) / 8 + 1) + x / 8;
				bool visible = pointer_mask[offset] & (1 << (x % 8));
				
				if (visible) {
					pixel = (pointer[offset] & (1 << (x % 8))) ?
					    0x000000 : 0xffffff;
					put_pixel(dx, dy, pixel);
				}
			}
		}
	}
}

static void pointer_hide(void)
{
	if (kfb.pointer_visible) {
		for (sysarg_t y = 0; y < POINTER_HEIGHT; y++) {
			for (sysarg_t x = 0; x < POINTER_WIDTH; x++) {
				sysarg_t dx = kfb.pointer_x + x;
				sysarg_t dy = kfb.pointer_y + y;
				
				pixel_t pixel =
				    imgmap_get_pixel(kfb.pointer_imgmap, x, y);
				put_pixel(dx, dy, pixel);
			}
		}
	}
}

/** Draw a filled rectangle.
 *
 */
static void draw_filled_rect(sysarg_t x1, sysarg_t y1, sysarg_t x2, sysarg_t y2,
    pixel_t color)
{
	if ((y1 >= y2) || (x1 >= x2))
		return;
	
	uint8_t cbuf[4];
	kfb.pixel2visual(cbuf, color);
	
	for (sysarg_t y = y1; y < y2; y++) {
		uint8_t *dst = kfb.addr + FB_POS(x1, y);
		
		for (sysarg_t x = x1; x < x2; x++) {
			memcpy(dst, cbuf, kfb.pixel_bytes);
			dst += kfb.pixel_bytes;
		}
	}
}

static void vp_put_pixel(fbvp_t *vp, sysarg_t x, sysarg_t y, pixel_t pixel)
{
	put_pixel(vp->x + x, vp->y + y, pixel);
}

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

/** Draw a character, takes advantage of alignment.
 *
 * This version can only be used if the following conditions are met:
 *
 *   - word size is divisible by pixel_bytes
 *   - cell scanline size is divisible by word size
 *   - cell scanlines are word-aligned
 *
 * It makes use of the pre-rendered mask to process (possibly) several
 * pixels at once (word size / pixel_bytes pixels at a time are processed)
 * making it very fast. Most notably this version is not applicable at 24 bits
 * per pixel.
 *
 * @param x        X coordinate of top-left corner on screen.
 * @param y        Y coordinate of top-left corner on screen.
 * @param inverted Draw inverted character.
 * @param ch       Character to draw.
 * @param bgcolor  Background color.
 * @param fbgcolor Foreground color.
 *
 */
static void draw_char_aligned(sysarg_t x, sysarg_t y, bool inverted, wchar_t ch,
    pixel_t bgcolor, pixel_t fgcolor)
{
	size_t word_size = sizeof(unsigned long);
	
	/*
	 * Prepare a pair of words, one filled with foreground-color
	 * pattern and the other filled with background-color pattern.
	 */
	unsigned long fg_buf;
	unsigned long bg_buf;
	
	for (size_t i = 0; i < word_size / kfb.pixel_bytes; i++) {
		kfb.pixel2visual(&((uint8_t *) &bg_buf)[i * kfb.pixel_bytes],
		    bgcolor);
		kfb.pixel2visual(&((uint8_t *) &fg_buf)[i * kfb.pixel_bytes],
		    fgcolor);
	}
	
	/* Pointer to the current position in the mask. */
	unsigned long *maskp =
	    (unsigned long *) &kfb.glyphs[GLYPH_POS(
	    fb_font_glyph(ch), 0, inverted)];
	
	/* Pointer to the current position on the screen. */
	unsigned long *dst =
	    (unsigned long *) &kfb.addr[FB_POS(x, y)];
	
	/* Width of the character cell in words. */
	size_t ww = FONT_WIDTH * kfb.pixel_bytes / word_size;
	
	/* Offset to add when moving to another screen scanline. */
	size_t d_add = kfb.scanline - FONT_WIDTH * kfb.pixel_bytes;
	
	for (size_t yd = 0; yd < FONT_SCANLINES; yd++) {
		/*
		 * Now process the cell scanline, combining foreground
		 * and background color patters using the pre-rendered mask.
		 */
		for (size_t i = 0; i < ww; i++) {
			unsigned long mask = *maskp++;
			*dst++ = (fg_buf & mask) | (bg_buf & ~mask);
		}
		
		/* Move to the beginning of the next scanline of the cell. */
		dst = (unsigned long *) ((uint8_t *) dst + d_add);
	}
}

/** Draw a character, fallback version.
 *
 * This version does not make use of the pre-rendered mask, it uses
 * the font bitmap directly. It works always, but it is slower.
 *
 * @param x        X coordinate of top-left corner on screen.
 * @param y        Y coordinate of top-left corner on screen.
 * @param inverted Draw inverted character.
 * @param ch       Character to draw.
 * @param bgcolor  Background color.
 * @param fgcolor  Foreground color.
 *
 */
static void draw_char_fallback(sysarg_t x, sysarg_t y, bool inverted,
    wchar_t ch, pixel_t bgcolor, pixel_t fgcolor)
{
	/* Character glyph */
	uint16_t glyph = fb_font_glyph(ch);
	
	/* Pre-render the foreground and background color pixels. */
	uint8_t fg_buf[4];
	uint8_t bg_buf[4];
	
	if (inverted) {
		kfb.pixel2visual(bg_buf, fgcolor);
		kfb.pixel2visual(fg_buf, bgcolor);
	} else {
		kfb.pixel2visual(bg_buf, bgcolor);
		kfb.pixel2visual(fg_buf, fgcolor);
	}
	
	/* Pointer to the current position on the screen. */
	uint8_t *dst = (uint8_t *) &kfb.addr[FB_POS(x, y)];
	
	/* Offset to add when moving to another screen scanline. */
	size_t d_add = kfb.scanline - FONT_WIDTH * kfb.pixel_bytes;
	
	for (size_t yd = 0; yd < FONT_SCANLINES; yd++) {
		/* Byte containing bits of the glyph scanline. */
		uint8_t byte = fb_font[glyph][yd];
		
		for (size_t i = 0; i < FONT_WIDTH; i++) {
			/* Choose color based on the current bit. */
			uint8_t *src = (byte & 0x80) ? fg_buf : bg_buf;
			
			/* Copy the pixel. */
			for (size_t j = 0; j < kfb.pixel_bytes; j++)
				*dst++ = *src++;
			
			/* Move to the next bit. */
			byte <<= 1;
		}
		
		/* Move to the beginning of the next scanline of the cell. */
		dst += d_add;
	}
}

/** Draw the character at the specified position in viewport.
 *
 * @param vp     Viewport.
 * @param col    Screen position relative to viewport.
 * @param row    Screen position relative to viewport.
 *
 */
static void draw_vp_char(fbvp_t *vp, sysarg_t col, sysarg_t row)
{
	kfb_vp_t *kfb_vp = (kfb_vp_t *) vp->data;
	
	sysarg_t x = vp->x + COL2X(col);
	sysarg_t y = vp->y + ROW2Y(row);
	
	charfield_t *field = screenbuffer_field_at(vp->backbuf, col, row);
	
	pixel_t bgcolor = 0;
	pixel_t fgcolor = 0;
	attrs_rgb(field->attrs, &bgcolor, &fgcolor);
	
	bool inverted = (vp->cursor_flash) &&
	    screenbuffer_cursor_at(vp->backbuf, col, row);
	
	(*kfb_vp->draw_char)(x, y, inverted, field->ch, bgcolor, fgcolor);
}

static errno_t kfb_yield(fbdev_t *dev)
{
	if (kfb.backbuf == NULL) {
		kfb.backbuf =
		    malloc(kfb.width * kfb.height * kfb.pixel_bytes);
		if (kfb.backbuf == NULL)
			return ENOMEM;
	}
	
	for (sysarg_t y = 0; y < kfb.height; y++)
		memcpy(kfb.backbuf + y * kfb.width * kfb.pixel_bytes,
		    kfb.addr + FB_POS(0, y), kfb.width * kfb.pixel_bytes);
	
	return EOK;
}

static errno_t kfb_claim(fbdev_t *dev)
{
	if (kfb.backbuf == NULL)
		return ENOENT;
	
	for (sysarg_t y = 0; y < kfb.height; y++)
		memcpy(kfb.addr + FB_POS(0, y),
		    kfb.backbuf + y * kfb.width * kfb.pixel_bytes,
		    kfb.width * kfb.pixel_bytes);
	
	return EOK;
}

static void kfb_pointer_update(struct fbdev *dev, sysarg_t x, sysarg_t y,
    bool visible)
{
	pointer_hide();
	
	kfb.pointer_x = x;
	kfb.pointer_y = y;
	kfb.pointer_visible = visible;
	
	pointer_show();
}

static errno_t kfb_get_resolution(fbdev_t *dev, sysarg_t *width, sysarg_t *height)
{
	*width = kfb.width;
	*height = kfb.height;
	return EOK;
}

static void kfb_font_metrics(fbdev_t *dev, sysarg_t width, sysarg_t height,
    sysarg_t *cols, sysarg_t *rows)
{
	*cols = X2COL(width);
	*rows = Y2ROW(height);
}

static errno_t kfb_vp_create(fbdev_t *dev, fbvp_t *vp)
{
	kfb_vp_t *kfb_vp = malloc(sizeof(kfb_vp_t));
	if (kfb_vp == NULL)
		return ENOMEM;
	
	/*
	 * Conditions necessary to select aligned glyph
	 * drawing variants:
	 *  - word size is divisible by number of bytes per pixel
	 *  - cell scanline size is divisible by word size
	 *  - cell scanlines are word-aligned
	 *
	 */
	size_t word_size = sizeof(unsigned long);
	
	if (((word_size % kfb.pixel_bytes) == 0)
	    && ((FONT_WIDTH * kfb.pixel_bytes) % word_size == 0)
	    && ((vp->x * kfb.pixel_bytes) % word_size == 0)
	    && (kfb.scanline % word_size == 0))
		kfb_vp->draw_char = draw_char_aligned;
	else
		kfb_vp->draw_char = draw_char_fallback;
	
	vp->attrs.type = CHAR_ATTR_RGB;
	vp->attrs.val.rgb.bgcolor = DEFAULT_BGCOLOR;
	vp->attrs.val.rgb.fgcolor = DEFAULT_FGCOLOR;
	vp->data = (void *) kfb_vp;
	
	return EOK;
}

static void kfb_vp_destroy(fbdev_t *dev, fbvp_t *vp)
{
	free(vp->data);
}

static void kfb_vp_clear(fbdev_t *dev, fbvp_t *vp)
{
	pointer_hide();
	
	for (sysarg_t row = 0; row < vp->rows; row++) {
		for (sysarg_t col = 0; col < vp->cols; col++) {
			charfield_t *field =
			    screenbuffer_field_at(vp->backbuf, col, row);
			
			field->ch = 0;
			field->attrs = vp->attrs;
		}
	}
	
	pixel_t bgcolor = 0;
	pixel_t fgcolor = 0;
	attrs_rgb(vp->attrs, &bgcolor, &fgcolor);
	
	draw_filled_rect(vp->x, vp->y, vp->x + vp->width,
	    vp->y + vp->height, bgcolor);
	
	pointer_show();
}

static console_caps_t kfb_vp_get_caps(fbdev_t *dev, fbvp_t *vp)
{
	return (CONSOLE_CAP_STYLE | CONSOLE_CAP_INDEXED | CONSOLE_CAP_RGB);
}

static void kfb_vp_cursor_update(fbdev_t *dev, fbvp_t *vp, sysarg_t prev_col,
    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible)
{
	pointer_hide();
	draw_vp_char(vp, prev_col, prev_row);
	draw_vp_char(vp, col, row);
	pointer_show();
}

static void kfb_vp_cursor_flash(fbdev_t *dev, fbvp_t *vp, sysarg_t col,
    sysarg_t row)
{
	pointer_hide();
	draw_vp_char(vp, col, row);
	pointer_show();
}

static void kfb_vp_char_update(fbdev_t *dev, fbvp_t *vp, sysarg_t col,
    sysarg_t row)
{
	pointer_hide();
	draw_vp_char(vp, col, row);
	pointer_show();
}

static void kfb_vp_imgmap_damage(fbdev_t *dev, fbvp_t *vp, imgmap_t *imgmap,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height)
{
	pointer_hide();
	
	for (sysarg_t y = 0; y < height; y++) {
		for (sysarg_t x = 0; x < width; x++) {
			pixel_t pixel = imgmap_get_pixel(imgmap, x0 + x, y0 + y);
			vp_put_pixel(vp, x0 + x, y0 + y, pixel);
		}
	}
	
	pointer_show();
}

static fbdev_ops_t kfb_ops = {
	.yield = kfb_yield,
	.claim = kfb_claim,
	.pointer_update = kfb_pointer_update,
	.get_resolution = kfb_get_resolution,
	.font_metrics = kfb_font_metrics,
	.vp_create = kfb_vp_create,
	.vp_destroy = kfb_vp_destroy,
	.vp_clear = kfb_vp_clear,
	.vp_get_caps = kfb_vp_get_caps,
	.vp_cursor_update = kfb_vp_cursor_update,
	.vp_cursor_flash = kfb_vp_cursor_flash,
	.vp_char_update = kfb_vp_char_update,
	.vp_imgmap_damage = kfb_vp_imgmap_damage
};

/** Render glyphs
 *
 * Convert glyphs from device independent font
 * description to current visual representation.
 *
 */
static void render_glyphs(size_t sz)
{
	memset(kfb.glyphs, 0, sz);
	
	for (unsigned int glyph = 0; glyph < FONT_GLYPHS; glyph++) {
		for (unsigned int y = 0; y < FONT_SCANLINES; y++) {
			for (unsigned int x = 0; x < FONT_WIDTH; x++) {
				kfb.visual_mask(
				    &kfb.glyphs[GLYPH_POS(glyph, y, false) + x * kfb.pixel_bytes],
				    (fb_font[glyph][y] & (1 << (7 - x))) ? true : false);
				kfb.visual_mask(
				    &kfb.glyphs[GLYPH_POS(glyph, y, true) + x * kfb.pixel_bytes],
				    (fb_font[glyph][y] & (1 << (7 - x))) ? false : true);
			}
		}
	}
}

errno_t kfb_init(void)
{
	sysarg_t present;
	errno_t rc = sysinfo_get_value("fb", &present);
	if (rc != EOK)
		present = false;
	
	if (!present)
		return ENOENT;
	
	sysarg_t kind;
	rc = sysinfo_get_value("fb.kind", &kind);
	if (rc != EOK)
		kind = (sysarg_t) -1;
	
	if (kind != 1)
		return EINVAL;
	
	sysarg_t paddr;
	rc = sysinfo_get_value("fb.address.physical", &paddr);
	if (rc != EOK)
		return rc;
	
	sysarg_t offset;
	rc = sysinfo_get_value("fb.offset", &offset);
	if (rc != EOK)
		offset = 0;
	
	sysarg_t width;
	rc = sysinfo_get_value("fb.width", &width);
	if (rc != EOK)
		return rc;
	
	sysarg_t height;
	rc = sysinfo_get_value("fb.height", &height);
	if (rc != EOK)
		return rc;
	
	sysarg_t scanline;
	rc = sysinfo_get_value("fb.scanline", &scanline);
	if (rc != EOK)
		return rc;
	
	sysarg_t visual;
	rc = sysinfo_get_value("fb.visual", &visual);
	if (rc != EOK)
		return rc;
	
	kfb.width = width;
	kfb.height = height;
	kfb.offset = offset;
	kfb.scanline = scanline;
	kfb.visual = visual;
	
	switch (visual) {
	case VISUAL_INDIRECT_8:
		kfb.pixel2visual = pixel2bgr_323;
		kfb.visual2pixel = bgr_323_2pixel;
		kfb.visual_mask = visual_mask_323;
		kfb.pixel_bytes = 1;
		break;
	case VISUAL_RGB_5_5_5_LE:
		kfb.pixel2visual = pixel2rgb_555_le;
		kfb.visual2pixel = rgb_555_le_2pixel;
		kfb.visual_mask = visual_mask_555;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_5_5_BE:
		kfb.pixel2visual = pixel2rgb_555_be;
		kfb.visual2pixel = rgb_555_be_2pixel;
		kfb.visual_mask = visual_mask_555;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_LE:
		kfb.pixel2visual = pixel2rgb_565_le;
		kfb.visual2pixel = rgb_565_le_2pixel;
		kfb.visual_mask = visual_mask_565;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_5_6_5_BE:
		kfb.pixel2visual = pixel2rgb_565_be;
		kfb.visual2pixel = rgb_565_be_2pixel;
		kfb.visual_mask = visual_mask_565;
		kfb.pixel_bytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		kfb.pixel2visual = pixel2rgb_888;
		kfb.visual2pixel = rgb_888_2pixel;
		kfb.visual_mask = visual_mask_888;
		kfb.pixel_bytes = 3;
		break;
	case VISUAL_BGR_8_8_8:
		kfb.pixel2visual = pixel2bgr_888;
		kfb.visual2pixel = bgr_888_2pixel;
		kfb.visual_mask = visual_mask_888;
		kfb.pixel_bytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		kfb.pixel2visual = pixel2rgb_8880;
		kfb.visual2pixel = rgb_8880_2pixel;
		kfb.visual_mask = visual_mask_8880;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		kfb.pixel2visual = pixel2rgb_0888;
		kfb.visual2pixel = rgb_0888_2pixel;
		kfb.visual_mask = visual_mask_0888;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		kfb.pixel2visual = pixel2bgr_0888;
		kfb.visual2pixel = bgr_0888_2pixel;
		kfb.visual_mask = visual_mask_0888;
		kfb.pixel_bytes = 4;
		break;
	case VISUAL_BGR_8_8_8_0:
		kfb.pixel2visual = pixel2bgr_8880;
		kfb.visual2pixel = bgr_8880_2pixel;
		kfb.visual_mask = visual_mask_8880;
		kfb.pixel_bytes = 4;
		break;
	default:
		return EINVAL;
	}
	
	kfb.glyph_scanline = FONT_WIDTH * kfb.pixel_bytes;
	kfb.glyph_bytes = kfb.glyph_scanline * FONT_SCANLINES;
	
	size_t sz = 2 * FONT_GLYPHS * kfb.glyph_bytes;
	kfb.glyphs = (uint8_t *) malloc(sz);
	if (kfb.glyphs == NULL)
		return EINVAL;
	
	render_glyphs(sz);
	
	kfb.size = scanline * height;
	
	rc = physmem_map((void *) paddr + offset,
	    ALIGN_UP(kfb.size, PAGE_SIZE) >> PAGE_WIDTH,
	    AS_AREA_READ | AS_AREA_WRITE, (void *) &kfb.addr);
	if (rc != EOK) {
		free(kfb.glyphs);
		return rc;
	}
	
	kfb.pointer_x = 0;
	kfb.pointer_y = 0;
	kfb.pointer_visible = false;
	kfb.pointer_imgmap = imgmap_create(POINTER_WIDTH, POINTER_HEIGHT,
	    VISUAL_RGB_0_8_8_8, IMGMAP_FLAG_NONE);
	
	kfb.backbuf = NULL;
	
	fbdev_t *dev = fbdev_register(&kfb_ops, (void *) &kfb);
	if (dev == NULL) {
		free(kfb.glyphs);
		as_area_destroy(kfb.addr);
		return EINVAL;
	}
	
	return EOK;
}

/**
 * @}
 */
