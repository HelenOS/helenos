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

#include <genarch/fb/font-8x16.h>
#include <genarch/fb/fb.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <mm/slab.h>
#include <panic.h>
#include <memstr.h>
#include <config.h>

#include "helenos.xbm"

SPINLOCK_INITIALIZE(fb_lock);

static __u8 *fbaddress = NULL;

static __u8 *blankline = NULL;

static unsigned int xres = 0;
static unsigned int yres = 0;
static unsigned int scanline = 0;
static unsigned int bitspp = 0;
static unsigned int pixelbytes = 0;

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

#define POINTPOS(x, y)	(y * scanline + x * pixelbytes)

/***************************************************************/
/* Pixel specific fuctions */

static void (*putpixel)(unsigned int x, unsigned int y, int color);
static int (*getpixel)(unsigned int x, unsigned int y);

/** Put pixel - 24-bit depth, 1 free byte */
static void putpixel_4byte(unsigned int x, unsigned int y, int color)
{
	*((__u32 *)(fbaddress + POINTPOS(x, y))) = color;
}

/** Return pixel color - 24-bit depth, 1 free byte */
static int getpixel_4byte(unsigned int x, unsigned int y)
{
	return *((__u32 *)(fbaddress + POINTPOS(x, y))) & 0xffffff;
}

/** Put pixel - 24-bit depth */
static void putpixel_3byte(unsigned int x, unsigned int y, int color)
{
	unsigned int startbyte = POINTPOS(x, y);

#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	fbaddress[startbyte] = RED(color, 8);
	fbaddress[startbyte + 1] = GREEN(color, 8);
	fbaddress[startbyte + 2] = BLUE(color, 8);
#else
	fbaddress[startbyte + 2] = RED(color, 8);
	fbaddress[startbyte + 1] = GREEN(color, 8);
	fbaddress[startbyte + 0] = BLUE(color, 8);
#endif	
}

/** Return pixel color - 24-bit depth */
static int getpixel_3byte(unsigned int x, unsigned int y)
{
	unsigned int startbyte = POINTPOS(x, y);

#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	return fbaddress[startbyte] << 16 | fbaddress[startbyte + 1] << 8 | fbaddress[startbyte + 2];
#else
	return fbaddress[startbyte + 2] << 16 | fbaddress[startbyte + 1] << 8 | fbaddress[startbyte + 0];
#endif	
}

/** Put pixel - 16-bit depth (5:6:6) */
static void putpixel_2byte(unsigned int x, unsigned int y, int color)
{
	/* 5-bit, 5-bits, 5-bits */
	*((__u16 *)(fbaddress + POINTPOS(x, y))) = RED(color, 5) << 11 | GREEN(color, 6) << 5 | BLUE(color, 5);
}

/** Return pixel color - 16-bit depth (5:6:6) */
static int getpixel_2byte(unsigned int x, unsigned int y)
{
	int color = *((__u16 *)(fbaddress + POINTPOS(x, y)));
	return (((color >> 11) & 0x1f) << (16 + 3)) | (((color >> 5) & 0x3f) << (8 + 2)) | ((color & 0x1f) << 3);
}

/** Put pixel - 8-bit depth (3:2:3) */
static void putpixel_1byte(unsigned int x, unsigned int y, int color)
{
	fbaddress[POINTPOS(x, y)] = RED(color, 3) << 5 | GREEN(color, 2) << 3 | BLUE(color, 3);
}

/** Return pixel color - 8-bit depth (3:2:3) */
static int getpixel_1byte(unsigned int x, unsigned int y)
{
	int color = fbaddress[POINTPOS(x, y)];
	return (((color >> 5) & 0x7) << (16 + 5)) | (((color >> 3) & 0x3) << (8 + 6)) | ((color & 0x7) << 5);
}

/** Fill line with color BGCOLOR */
static void clear_line(unsigned int y)
{
	unsigned int x;
	
	for (x = 0; x < xres; x++)
		(*putpixel)(x, y, BGCOLOR);
}


/** Fill screen with background color */
static void clear_screen(void)
{
	unsigned int y;

	for (y = 0; y < yres; y++)
		clear_line(y);
}


/** Scroll screen one row up */
static void scroll_screen(void)
{
	unsigned int i;
	__u8 *lastline = &fbaddress[(rows - 1) * ROW_BYTES];

	memcpy((void *) fbaddress, (void *) &fbaddress[ROW_BYTES], scanline * yres - ROW_BYTES);

	/* Clear last row */
	if (blankline) {
		memcpy((void *) lastline, (void *) blankline, ROW_BYTES);
	} else {
		for (i = 0; i < FONT_SCANLINES; i++)
			clear_line((rows - 1) * FONT_SCANLINES + i);

		if (config.mm_initialized) {
			/* Save a blank line aside. */
			blankline = (__u8 *) malloc(ROW_BYTES, FRAME_ATOMIC);
			if (blankline)
				memcpy((void *) blankline, (void *) lastline, ROW_BYTES);
		}
	}
}


static void invert_pixel(unsigned int x, unsigned int y)
{
	(*putpixel)(x, y, ~(*getpixel)(x, y));
}


/** Draw one line of glyph at a given position */
static void draw_glyph_line(unsigned int glline, unsigned int x, unsigned int y)
{
	unsigned int i;

	for (i = 0; i < 8; i++)
		if (glline & (1 << (7 - i))) {
			(*putpixel)(x + i, y, FGCOLOR);
		} else
			(*putpixel)(x + i, y, BGCOLOR);
}

/***************************************************************/
/* Character-console functions */

/** Draw character at given position */
static void draw_glyph(__u8 glyph, unsigned int col, unsigned int row)
{
	unsigned int y;

	for (y = 0; y < FONT_SCANLINES; y++)
		draw_glyph_line(fb_font[glyph * FONT_SCANLINES + y], col * COL_WIDTH, row * FONT_SCANLINES + y);
}

/** Invert character at given position */
static void invert_char(unsigned int col, unsigned int row)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < COL_WIDTH; x++)
		for (y = 0; y < FONT_SCANLINES; y++)
			invert_pixel(col * COL_WIDTH + x, row * FONT_SCANLINES + y);
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
				(*putpixel)(startx + x, starty + y, LOGOCOLOR);
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
 * @param addr Address of theframebuffer
 * @param x    Screen width in pixels
 * @param y    Screen height in pixels
 * @param bpp  Bits per pixel (8, 16, 24, 32)
 * @param scan Bytes per one scanline
 *
 */
void fb_init(__address addr, unsigned int x, unsigned int y, unsigned int bpp, unsigned int scan)
{
	switch (bpp) {
		case 8:
			putpixel = putpixel_1byte;
			getpixel = getpixel_1byte;
			pixelbytes = 1;
			break;
		case 16:
			putpixel = putpixel_2byte;
			getpixel = getpixel_2byte;
			pixelbytes = 2;
			break;
		case 24:
			putpixel = putpixel_3byte;
			getpixel = getpixel_3byte;
			pixelbytes = 3;
			break;
		case 32:
			putpixel = putpixel_4byte;
			getpixel = getpixel_4byte;
			pixelbytes = 4;
			break;
		default:
			panic("Unsupported bpp");
	}
		
	fbaddress = (unsigned char *) addr;
	xres = x;
	yres = y;
	bitspp = bpp;
	scanline = scan;
	
	rows = y / FONT_SCANLINES;
	columns = x / COL_WIDTH;

	clear_screen();
	draw_logo(xres - helenos_width, 0);
	invert_cursor();

	chardev_initialize("fb", &framebuffer, &fb_ops);
	stdout = &framebuffer;
}


/** Register framebuffer in sysinfo
 *
 */
void fb_register(void)
{
	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.width", NULL, xres);
	sysinfo_set_item_val("fb.height", NULL, yres);
	sysinfo_set_item_val("fb.scanline", NULL, scanline);
	sysinfo_set_item_val("fb.bpp", NULL, bitspp);
	sysinfo_set_item_val("fb.address.virtual", NULL, (__address) fbaddress);
}
