/*
 * Copyright (c) 2008 Martin Decky
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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#include <genarch/fb/font-8x16.h>
#include <genarch/fb/logo-196x66.h>
#include <genarch/fb/visuals.h>
#include <genarch/fb/fb.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <mm/slab.h>
#include <align.h>
#include <panic.h>
#include <memstr.h>
#include <config.h>
#include <bitops.h>
#include <print.h>
#include <string.h>
#include <ddi/ddi.h>
#include <arch/types.h>
#include <byteorder.h>

SPINLOCK_INITIALIZE(fb_lock);

static uint8_t *fb_addr;
static uint16_t *backbuf;
static uint8_t *glyphs;
static uint8_t *bgscan;

static unsigned int xres;
static unsigned int yres;

static unsigned int ylogo;
static unsigned int ytrim;
static unsigned int rowtrim;

static unsigned int scanline;
static unsigned int glyphscanline;

static unsigned int pixelbytes;
static unsigned int glyphbytes;
static unsigned int bgscanbytes;

static unsigned int cols;
static unsigned int rows;
static unsigned int position = 0;

#define BG_COLOR     0x000080
#define FG_COLOR     0xffff00
#define INV_COLOR    0xaaaaaa

#define RED(x, bits)         (((x) >> (8 + 8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define GREEN(x, bits)       (((x) >> (8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define BLUE(x, bits)        (((x) >> (8 - (bits))) & ((1 << (bits)) - 1))

#define COL2X(col)           ((col) * FONT_WIDTH)
#define ROW2Y(row)           ((row) * FONT_SCANLINES)

#define X2COL(x)             ((x) / FONT_WIDTH)
#define Y2ROW(y)             ((y) / FONT_SCANLINES)

#define FB_POS(x, y)         ((y) * scanline + (x) * pixelbytes)
#define BB_POS(col, row)     ((row) * cols + (col))
#define GLYPH_POS(glyph, y)  ((glyph) * glyphbytes + (y) * glyphscanline)


static void (*rgb_conv)(void *, uint32_t);

/*
 * RGB conversion functions.
 *
 * These functions write an RGB value to some memory in some predefined format.
 * The naming convention corresponds to the format created by these functions.
 * The functions use the so called network order (i.e. big endian) with respect
 * to their names.
 */

static void rgb_0888(void *dst, uint32_t rgb)
{
	*((uint32_t *) dst) = host2uint32_t_be((0 << 24) |
	    (RED(rgb, 8) << 16) | (GREEN(rgb, 8) << 8) | (BLUE(rgb, 8)));
}

static void bgr_0888(void *dst, uint32_t rgb)
{
	*((uint32_t *) dst) = host2uint32_t_be((0 << 24) |
	    (BLUE(rgb, 8) << 16) | (GREEN(rgb, 8) << 8) | (RED(rgb, 8)));
}

static void rgb_8880(void *dst, uint32_t rgb)
{
	*((uint32_t *) dst) = host2uint32_t_be((RED(rgb, 8) << 24) |
	    (GREEN(rgb,	8) << 16) | (BLUE(rgb, 8) << 8) | 0);
}

static void bgr_8880(void *dst, uint32_t rgb)
{
	*((uint32_t *) dst) = host2uint32_t_be((BLUE(rgb, 8) << 24) |
	    (GREEN(rgb,	8) << 16) | (RED(rgb, 8) << 8) | 0);
}

static void rgb_888(void *dst, uint32_t rgb)
{
	((uint8_t *) dst)[0] = RED(rgb, 8);
	((uint8_t *) dst)[1] = GREEN(rgb, 8);
	((uint8_t *) dst)[2] = BLUE(rgb, 8);
}

static void bgr_888(void *dst, uint32_t rgb)
{
	((uint8_t *) dst)[0] = BLUE(rgb, 8);
	((uint8_t *) dst)[1] = GREEN(rgb, 8);
	((uint8_t *) dst)[2] = RED(rgb, 8);
}

static void bgr_555(void *dst, uint32_t rgb)
{
	uint8_t hi = (BLUE(rgb, 5) | (GREEN(rgb, 5) << 5)) & 0xff;
	uint8_t lo = (GREEN(rgb, 5) >> 3) | (RED(rgb, 5) << 2);
	*((uint16_t *) dst) = host2uint16_t_be((hi << 8) | lo);
}

static void bgr_565(void *dst, uint32_t rgb)
{
	uint8_t hi = (BLUE(rgb, 5) | (GREEN(rgb, 6) << 5)) & 0xff;
	uint8_t lo = (GREEN(rgb, 6) >> 3) | (RED(rgb, 5) << 3);
	*((uint16_t *) dst) = host2uint16_t_be((hi << 8) | lo);
}


/** BGR 3:2:3
 *
 * Even though we try 3:2:3 color scheme here, an 8-bit framebuffer
 * will most likely use a color palette. The color appearance
 * will be pretty random and depend on the default installed
 * palette. This could be fixed by supporting custom palette
 * and setting it to simulate the 8-bit truecolor.
 *
 * Currently we set the palette on the ia32, amd64 and sparc64 port.
 *
 * Note that the byte is being inverted by this function. The reason is
 * that we would like to use a color palette where the white color code
 * is 0 and the black color code is 255, as some machines (Sun Blade 1500)
 * use these codes for black and white and prevent to set codes
 * 0 and 255 to other colors.
 *
 */
static void bgr_323(void *dst, uint32_t rgb)
{
	*((uint8_t *) dst)
	    = ~((RED(rgb, 3) << 5) | (GREEN(rgb, 2) << 3) | BLUE(rgb, 3));
}


/** Hide logo and refresh screen
 *
 */
static void logo_hide(bool silent)
{
	ylogo = 0;
	ytrim = yres;
	rowtrim = rows;
	if (!silent)
		fb_redraw();
}


/** Draw character at given position
 *
 */
static void glyph_draw(uint16_t glyph, unsigned int col, unsigned int row, bool silent, bool overlay)
{
	unsigned int x = COL2X(col);
	unsigned int y = ROW2Y(row);
	unsigned int yd;
	
	if (y >= ytrim)
		logo_hide(silent);
	
	if (!overlay)
		backbuf[BB_POS(col, row)] = glyph;
	
	if (!silent) {
		for (yd = 0; yd < FONT_SCANLINES; yd++)
			memcpy(&fb_addr[FB_POS(x, y + yd + ylogo)],
			    &glyphs[GLYPH_POS(glyph, yd)], glyphscanline);
	}
}


/** Scroll screen down by one row
 *
 *
 */
static void screen_scroll(bool silent)
{
	if (ylogo > 0) {
		logo_hide(silent);
		return;
	}
	
	if (!silent) {
		unsigned int row;
		
		for (row = 0; row < rows; row++) {
			unsigned int y = ROW2Y(row);
			unsigned int yd;
			
			for (yd = 0; yd < FONT_SCANLINES; yd++) {
				unsigned int x;
				unsigned int col;
				
				for (col = 0, x = 0; col < cols; col++,
				    x += FONT_WIDTH) {
					uint16_t glyph;
					
					if (row < rows - 1) {
						if (backbuf[BB_POS(col, row)] ==
						    backbuf[BB_POS(col, row + 1)])
							continue;
						
						glyph = backbuf[BB_POS(col, row + 1)];
					} else
						glyph = 0;
					
					memcpy(&fb_addr[FB_POS(x, y + yd)],
					    &glyphs[GLYPH_POS(glyph, yd)],
					    glyphscanline);
				}
			}
		}
	}
	
	memmove(backbuf, &backbuf[BB_POS(0, 1)], cols * (rows - 1) * sizeof(uint16_t));
	memsetw(&backbuf[BB_POS(0, rows - 1)], cols, 0);
}


static void cursor_put(bool silent)
{
	unsigned int col = position % cols;
	unsigned int row = position / cols;
	
	glyph_draw(fb_font_glyph(U_CURSOR), col, row, silent, true);
}


static void cursor_remove(bool silent)
{
	unsigned int col = position % cols;
	unsigned int row = position / cols;
	
	glyph_draw(backbuf[BB_POS(col, row)], col, row, silent, true);
}


/** Print character to screen
 *
 * Emulate basic terminal commands.
 *
 */
static void fb_putchar(outdev_t *dev, wchar_t ch, bool silent)
{
	spinlock_lock(&fb_lock);
	
	switch (ch) {
	case '\n':
		cursor_remove(silent);
		position += cols;
		position -= position % cols;
		break;
	case '\r':
		cursor_remove(silent);
		position -= position % cols;
		break;
	case '\b':
		cursor_remove(silent);
		if (position % cols)
			position--;
		break;
	case '\t':
		cursor_remove(silent);
		do {
			glyph_draw(fb_font_glyph(' '), position % cols,
			    position / cols, silent, false);
			position++;
		} while ((position % 8) && (position < cols * rows));
		break;
	default:
		glyph_draw(fb_font_glyph(ch), position % cols,
		    position / cols, silent, false);
		position++;
	}
	
	if (position >= cols * rows) {
		position -= cols;
		screen_scroll(silent);
	}
	
	cursor_put(silent);
	
	spinlock_unlock(&fb_lock);
}

static outdev_t fb_console;
static outdev_operations_t fb_ops = {
	.write = fb_putchar
};


/** Render glyphs
 *
 * Convert glyphs from device independent font
 * description to current visual representation.
 *
 */
static void glyphs_render(void)
{
	/* Prerender glyphs */
	uint16_t glyph;
	
	for (glyph = 0; glyph < FONT_GLYPHS; glyph++) {
		uint32_t fg_color;
		
		if (glyph == FONT_GLYPHS - 1)
			fg_color = INV_COLOR;
		else
			fg_color = FG_COLOR;
		
		unsigned int y;
		
		for (y = 0; y < FONT_SCANLINES; y++) {
			unsigned int x;
			
			for (x = 0; x < FONT_WIDTH; x++) {
				void *dst = &glyphs[GLYPH_POS(glyph, y) +
				    x * pixelbytes];
				uint32_t rgb = (fb_font[glyph][y] &
				    (1 << (7 - x))) ? fg_color : BG_COLOR;
				rgb_conv(dst, rgb);
			}
		}
	}
	
	/* Prerender background scanline */
	unsigned int x;
	
	for (x = 0; x < xres; x++)
		rgb_conv(&bgscan[x * pixelbytes], BG_COLOR);
}


/** Refresh the screen
 *
 */
void fb_redraw(void)
{
	if (ylogo > 0) {
		unsigned int y;
		
		for (y = 0; y < LOGO_HEIGHT; y++) {
			unsigned int x;
			
			for (x = 0; x < xres; x++)
				rgb_conv(&fb_addr[FB_POS(x, y)],
				    (x < LOGO_WIDTH) ?
				    fb_logo[y * LOGO_WIDTH + x] :
				    LOGO_COLOR);
		}
	}
	
	unsigned int row;
	
	for (row = 0; row < rowtrim; row++) {
		unsigned int y = ylogo + ROW2Y(row);
		unsigned int yd;
		
		for (yd = 0; yd < FONT_SCANLINES; yd++) {
			unsigned int x;
			unsigned int col;
			
			for (col = 0, x = 0; col < cols;
			    col++, x += FONT_WIDTH) {
				uint16_t glyph = backbuf[BB_POS(col, row)];
				void *dst = &fb_addr[FB_POS(x, y + yd)];
				void *src = &glyphs[GLYPH_POS(glyph, yd)];
				memcpy(dst, src, glyphscanline);
			}
		}
	}
	
	if (COL2X(cols) < xres) {
		unsigned int y;
		unsigned int size = (xres - COL2X(cols)) * pixelbytes;
		
		for (y = ylogo; y < yres; y++)
			memcpy(&fb_addr[FB_POS(COL2X(cols), y)], bgscan, size);
	}
	
	if (ROW2Y(rowtrim) + ylogo < yres) {
		unsigned int y;
		
		for (y = ROW2Y(rowtrim) + ylogo; y < yres; y++)
			memcpy(&fb_addr[FB_POS(0, y)], bgscan, bgscanbytes);
	}
}


/** Initialize framebuffer as a output character device
 *
 * @param addr   Physical address of the framebuffer
 * @param x      Screen width in pixels
 * @param y      Screen height in pixels
 * @param scan   Bytes per one scanline
 * @param visual Color model
 *
 */
void fb_init(fb_properties_t *props)
{
	switch (props->visual) {
	case VISUAL_INDIRECT_8:
		rgb_conv = bgr_323;
		pixelbytes = 1;
		break;
	case VISUAL_BGR_5_5_5:
		rgb_conv = bgr_555;
		pixelbytes = 2;
		break;
	case VISUAL_BGR_5_6_5:
		rgb_conv = bgr_565;
		pixelbytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		rgb_conv = rgb_888;
		pixelbytes = 3;
		break;
	case VISUAL_BGR_8_8_8:
		rgb_conv = bgr_888;
		pixelbytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		rgb_conv = rgb_8880;
		pixelbytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		rgb_conv = rgb_0888;
		pixelbytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		rgb_conv = bgr_0888;
		pixelbytes = 4;
		break;
	case VISUAL_BGR_8_8_8_0:
		rgb_conv = bgr_8880;
		pixelbytes = 4;
		break;
	default:
		panic("Unsupported visual.");
	}
	
	xres = props->x;
	yres = props->y;
	scanline = props->scan;
	
	cols = X2COL(xres);
	rows = Y2ROW(yres);
	
	if (yres > ylogo) {
		ylogo = LOGO_HEIGHT;
		rowtrim = rows - Y2ROW(ylogo);
		if (ylogo % FONT_SCANLINES > 0)
			rowtrim--;
		ytrim = ROW2Y(rowtrim);
	} else {
		ylogo = 0;
		ytrim = yres;
		rowtrim = rows;
	}
	
	glyphscanline = FONT_WIDTH * pixelbytes;
	glyphbytes = ROW2Y(glyphscanline);
	bgscanbytes = xres * pixelbytes;
	
	size_t fbsize = scanline * yres;
	size_t bbsize = cols * rows * sizeof(uint16_t);
	size_t glyphsize = FONT_GLYPHS * glyphbytes;
	
	backbuf = (uint16_t *) malloc(bbsize, 0);
	if (!backbuf)
		panic("Unable to allocate backbuffer.");
	
	glyphs = (uint8_t *) malloc(glyphsize, 0);
	if (!glyphs)
		panic("Unable to allocate glyphs.");
	
	bgscan = malloc(bgscanbytes, 0);
	if (!bgscan)
		panic("Unable to allocate background pixel.");
	
	memsetw(backbuf, cols * rows, 0);
	
	glyphs_render();
	
	fb_addr = (uint8_t *) hw_map((uintptr_t) props->addr, fbsize);
	
	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.kind", NULL, 1);
	sysinfo_set_item_val("fb.width", NULL, xres);
	sysinfo_set_item_val("fb.height", NULL, yres);
	sysinfo_set_item_val("fb.scanline", NULL, scanline);
	sysinfo_set_item_val("fb.visual", NULL, props->visual);
	sysinfo_set_item_val("fb.address.physical", NULL, props->addr);
	
	fb_redraw();
	
	outdev_initialize("fb", &fb_console, &fb_ops);
	stdout = &fb_console;
}

/** @}
 */
