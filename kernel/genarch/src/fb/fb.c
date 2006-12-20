/*
 * Copyright (C) 2006 Ondrej Palkovsky
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
#include <genarch/fb/visuals.h>
#include <genarch/fb/fb.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <mm/slab.h>
#include <panic.h>
#include <memstr.h>
#include <config.h>
#include <bitops.h>
#include <print.h>
#include <ddi/ddi.h>
#include <arch/types.h>
#include <typedefs.h>

#include "helenos.xbm"

static parea_t fb_parea;		/**< Physical memory area for fb. */

SPINLOCK_INITIALIZE(fb_lock);

static uint8_t *fbaddress = NULL;

static uint8_t *blankline = NULL;
static uint8_t *dbbuffer = NULL;	/* Buffer for fast scrolling console */
static index_t dboffset;

static unsigned int xres = 0;
static unsigned int yres = 0;
static unsigned int scanline = 0;
static unsigned int pixelbytes = 0;
#ifdef FB_INVERT_COLORS
static bool invert_colors = true;
#else
static bool invert_colors = false;
#endif

static unsigned int position = 0;
static unsigned int columns = 0;
static unsigned int rows = 0;

#define COL_WIDTH	8
#define ROW_BYTES	(scanline * FONT_SCANLINES)

#define BGCOLOR		0x000080
#define FGCOLOR		0xffff00
#define LOGOCOLOR	0x2020b0

#define RED(x, bits)	((x >> (16 + 8 - bits)) & ((1 << bits) - 1))
#define GREEN(x, bits)	((x >> (8 + 8 - bits)) & ((1 << bits) - 1))
#define BLUE(x, bits)	((x >> (8 - bits)) & ((1 << bits) - 1))

#define POINTPOS(x, y)	((y) * scanline + (x) * pixelbytes)

/***************************************************************/
/* Pixel specific fuctions */

static void (*rgb2scr)(void *, int);
static int (*scr2rgb)(void *);

static inline int COLOR(int color)
{
	return invert_colors ? ~color : color;
}

/* Conversion routines between different color representations */
static void rgb_byte0888(void *dst, int rgb)
{
	*((int *) dst) = rgb;
}

static int byte0888_rgb(void *src)
{
	return (*((int *) src)) & 0xffffff;
}

static void bgr_byte0888(void *dst, int rgb)
{
	*((uint32_t *) dst) = BLUE(rgb, 8) << 16 | GREEN(rgb, 8) << 8 |	RED(rgb,
		8);
}

static int byte0888_bgr(void *src)
{
	int color = *(uint32_t *)(src);
	return ((color & 0xff) << 16) | (((color >> 8) & 0xff) << 8) | ((color
		>> 16) & 0xff);
}

static void rgb_byte888(void *dst, int rgb)
{
	uint8_t *scr = dst;
#if defined(FB_INVERT_ENDIAN)
	scr[0] = RED(rgb, 8);
	scr[1] = GREEN(rgb, 8);
	scr[2] = BLUE(rgb, 8);
#else
	scr[2] = RED(rgb, 8);
	scr[1] = GREEN(rgb, 8);
	scr[0] = BLUE(rgb, 8);
#endif
}

static int byte888_rgb(void *src)
{
	uint8_t *scr = src;
#if defined(FB_INVERT_ENDIAN)
	return scr[0] << 16 | scr[1] << 8 | scr[2];
#else
	return scr[2] << 16 | scr[1] << 8 | scr[0];
#endif	
}

/**  16-bit depth (5:5:5) */
static void rgb_byte555(void *dst, int rgb)
{
	/* 5-bit, 5-bits, 5-bits */ 
	*((uint16_t *) dst) = RED(rgb, 5) << 10 | GREEN(rgb, 5) << 5 | BLUE(rgb,
		5);
}

/** 16-bit depth (5:5:5) */
static int byte555_rgb(void *src)
{
	int color = *(uint16_t *)(src);
	return (((color >> 10) & 0x1f) << (16 + 3)) | (((color >> 5) & 0x1f) <<
		(8 + 3)) | ((color & 0x1f) << 3);
}

/**  16-bit depth (5:6:5) */
static void rgb_byte565(void *dst, int rgb)
{
	/* 5-bit, 6-bits, 5-bits */ 
	*((uint16_t *) dst) = RED(rgb, 5) << 11 | GREEN(rgb, 6) << 5 | BLUE(rgb,		5);
}

/** 16-bit depth (5:6:5) */
static int byte565_rgb(void *src)
{
	int color = *(uint16_t *)(src);
	return (((color >> 11) & 0x1f) << (16 + 3)) | (((color >> 5) & 0x3f) <<
		(8 + 2)) | ((color & 0x1f) << 3);
}

/** Put pixel - 8-bit depth (color palette/3:2:3)
 *
 * Even though we try 3:2:3 color scheme here, an 8-bit framebuffer
 * will most likely use a color palette. The color appearance
 * will be pretty random and depend on the default installed
 * palette. This could be fixed by supporting custom palette
 * and setting it to simulate the 8-bit truecolor.
 */
static void rgb_byte8(void *dst, int rgb)
{
	*((uint8_t *) dst) = RED(rgb, 3) << 5 | GREEN(rgb, 2) << 3 | BLUE(rgb,
		3);
}

/** Return pixel color - 8-bit depth (color palette/3:2:3)
 *
 * See the comment for rgb_byte().
 */
static int byte8_rgb(void *src)
{
	int color = *(uint8_t *)src;
	return (((color >> 5) & 0x7) << (16 + 5)) | (((color >> 3) & 0x3) << (8
		+ 6)) | ((color & 0x7) << 5);
}

static void putpixel(unsigned int x, unsigned int y, int color)
{
	(*rgb2scr)(&fbaddress[POINTPOS(x, y)], COLOR(color));

	if (dbbuffer) {
		int dline = (y + dboffset) % yres;
		(*rgb2scr)(&dbbuffer[POINTPOS(x, dline)], COLOR(color));
	}
}

/** Get pixel from viewport */
static int getpixel(unsigned int x, unsigned int y)
{
	if (dbbuffer) {
		int dline = (y + dboffset) % yres;
		return COLOR((*scr2rgb)(&dbbuffer[POINTPOS(x, dline)]));
	}
	return COLOR((*scr2rgb)(&fbaddress[POINTPOS(x, y)]));
}


/** Fill screen with background color */
static void clear_screen(void)
{
	unsigned int y;

	for (y = 0; y < yres; y++) {
		memcpy(&fbaddress[scanline * y], blankline, xres * pixelbytes);
		if (dbbuffer)
			memcpy(&dbbuffer[scanline * y], blankline, xres *
				pixelbytes);
	}
}


/** Scroll screen one row up */
static void scroll_screen(void)
{
	if (dbbuffer) {
		count_t first;
		
		memcpy(&dbbuffer[dboffset * scanline], blankline, ROW_BYTES);
		
		dboffset = (dboffset + FONT_SCANLINES) % yres;
		first = yres - dboffset;

		memcpy(fbaddress, &dbbuffer[scanline * dboffset], first *
			scanline);
		memcpy(&fbaddress[first * scanline], dbbuffer, dboffset *
			scanline);
	} else {
		uint8_t *lastline = &fbaddress[(rows - 1) * ROW_BYTES];
		
		memcpy((void *) fbaddress, (void *) &fbaddress[ROW_BYTES],
			scanline * yres - ROW_BYTES);
		/* Clear last row */
		memcpy((void *) lastline, (void *) blankline, ROW_BYTES);
	}
}


static void invert_pixel(unsigned int x, unsigned int y)
{
	putpixel(x, y, ~getpixel(x, y));
}


/** Draw one line of glyph at a given position */
static void draw_glyph_line(unsigned int glline, unsigned int x, unsigned int y)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		if (glline & (1 << (7 - i))) {
			putpixel(x + i, y, FGCOLOR);
		} else
			putpixel(x + i, y, BGCOLOR);
}

/***************************************************************/
/* Character-console functions */

/** Draw character at given position */
static void draw_glyph(uint8_t glyph, unsigned int col, unsigned int row)
{
	unsigned int y;

	for (y = 0; y < FONT_SCANLINES; y++)
		draw_glyph_line(fb_font[glyph * FONT_SCANLINES + y], col *
			COL_WIDTH, row * FONT_SCANLINES + y);
}

/** Invert character at given position */
static void invert_char(unsigned int col, unsigned int row)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < COL_WIDTH; x++)
		for (y = 0; y < FONT_SCANLINES; y++)
			invert_pixel(col * COL_WIDTH + x, row * FONT_SCANLINES +
				y);
}

/** Draw character at default position */
static void draw_char(char chr)
{
	draw_glyph(chr, position % columns, position / columns);
}

static void draw_logo(unsigned int startx, unsigned int starty)
{
	unsigned int x;
	unsigned int y;
	unsigned int byte;
	unsigned int rowbytes;

	rowbytes = (helenos_width - 1) / 8 + 1;

	for (y = 0; y < helenos_height; y++)
		for (x = 0; x < helenos_width; x++) {
			byte = helenos_bits[rowbytes * y + x / 8];
			byte >>= x % 8;
			if (byte & 1)
				putpixel(startx + x, starty + y,
					COLOR(LOGOCOLOR));
		}
}

/***************************************************************/
/* Stdout specific functions */

static void invert_cursor(void)
{
	invert_char(position % columns, position / columns);
}

/** Print character to screen
 *
 *  Emulate basic terminal commands
 */
static void fb_putchar(chardev_t *dev, char ch)
{
	spinlock_lock(&fb_lock);
	
	switch (ch) {
	case '\n':
		invert_cursor();
		position += columns;
		position -= position % columns;
		break;
	case '\r':
		invert_cursor();
		position -= position % columns;
		break;
	case '\b':
		invert_cursor();
		if (position % columns)
			position--;
		break;
	case '\t':
		invert_cursor();
		do {
			draw_char(' ');
			position++;
		} while ((position % 8) && position < columns * rows);
		break;
	default:
		draw_char(ch);
		position++;
	}
	
	if (position >= columns * rows) {
		position -= columns;
		scroll_screen();
	}
	
	invert_cursor();
	
	spinlock_unlock(&fb_lock);
}

static chardev_t framebuffer;
static chardev_operations_t fb_ops = {
	.write = fb_putchar,
};


/** Initialize framebuffer as a chardev output device
 *
 * @param addr   Physical address of the framebuffer
 * @param x      Screen width in pixels
 * @param y      Screen height in pixels
 * @param scan   Bytes per one scanline
 * @param visual Color model
 *
 */
void fb_init(uintptr_t addr, unsigned int x, unsigned int y, unsigned int scan,
	unsigned int visual)
{
	switch (visual) {
	case VISUAL_INDIRECT_8:
		rgb2scr = rgb_byte8;
		scr2rgb = byte8_rgb;
		pixelbytes = 1;
		break;
	case VISUAL_RGB_5_5_5:
		rgb2scr = rgb_byte555;
		scr2rgb = byte555_rgb;
		pixelbytes = 2;
		break;
	case VISUAL_RGB_5_6_5:
		rgb2scr = rgb_byte565;
		scr2rgb = byte565_rgb;
		pixelbytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		rgb2scr = rgb_byte888;
		scr2rgb = byte888_rgb;
		pixelbytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		rgb2scr = rgb_byte888;
		scr2rgb = byte888_rgb;
		pixelbytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		rgb2scr = rgb_byte0888;
		scr2rgb = byte0888_rgb;
		pixelbytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		rgb2scr = bgr_byte0888;
		scr2rgb = byte0888_bgr;
		pixelbytes = 4;
		break;
	default:
		panic("Unsupported visual.\n");
	}
	
	unsigned int fbsize = scan * y;
	
	/* Map the framebuffer */
	fbaddress = (uint8_t *) hw_map((uintptr_t) addr, fbsize);
	
	xres = x;
	yres = y;
	scanline = scan;
	
	rows = y / FONT_SCANLINES;
	columns = x / COL_WIDTH;

	fb_parea.pbase = (uintptr_t) addr;
	fb_parea.vbase = (uintptr_t) fbaddress;
	fb_parea.frames = SIZE2FRAMES(fbsize);
	fb_parea.cacheable = false;
	ddi_parea_register(&fb_parea);

	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.kind", NULL, 1);
	sysinfo_set_item_val("fb.width", NULL, xres);
	sysinfo_set_item_val("fb.height", NULL, yres);
	sysinfo_set_item_val("fb.scanline", NULL, scan);
	sysinfo_set_item_val("fb.visual", NULL, visual);
	sysinfo_set_item_val("fb.address.physical", NULL, addr);
	sysinfo_set_item_val("fb.address.color", NULL, PAGE_COLOR((uintptr_t)
		fbaddress));
	sysinfo_set_item_val("fb.invert-colors", NULL, invert_colors);

	/* Allocate double buffer */
	unsigned int order = fnzb(SIZE2FRAMES(fbsize) - 1) + 1;
	dbbuffer = (uint8_t *) frame_alloc(order, FRAME_ATOMIC | FRAME_KA);
	if (!dbbuffer)
		printf("Failed to allocate scroll buffer.\n");
	dboffset = 0;

	/* Initialized blank line */
	blankline = (uint8_t *) malloc(ROW_BYTES, FRAME_ATOMIC);
	if (!blankline)
		panic("Failed to allocate blank line for framebuffer.");
	for (y = 0; y < FONT_SCANLINES; y++)
		for (x = 0; x < xres; x++)
			(*rgb2scr)(&blankline[POINTPOS(x, y)], COLOR(BGCOLOR));
	
	clear_screen();

	/* Update size of screen to match text area */
	yres = rows * FONT_SCANLINES;

	draw_logo(xres - helenos_width, 0);
	invert_cursor();

	chardev_initialize("fb", &framebuffer, &fb_ops);
	stdout = &framebuffer;
}

/** @}
 */
