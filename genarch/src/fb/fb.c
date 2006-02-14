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
#include <print.h>

SPINLOCK_INITIALIZE(fb_lock);

static __u8 *fbaddress=NULL;
static int xres,yres;
static int position=0;
static int columns=0;
static int rows=0;
static int pixelbytes=0;

#define COL_WIDTH   8
#define ROW_HEIGHT  (FONT_SCANLINES)
#define ROW_PIX     (xres*ROW_HEIGHT*pixelbytes)

#define BGCOLOR   0x000080
#define FGCOLOR   0xffff00

#define RED(x,bits)    ((x >> (16+8-bits)) & ((1<<bits)-1))
#define GREEN(x,bits)  ((x >> (8+8-bits)) & ((1<<bits)-1))
#define BLUE(x,bits)   ((x >> (8-bits)) & ((1<<bits)-1))

#define POINTPOS(x,y)   ((y*xres+x)*pixelbytes)

/***************************************************************/
/* Pixel specific fuctions */

/** Draw pixel of given color on screen */
static inline void putpixel(int x,int y,int color)
{
	int startbyte = POINTPOS(x,y);

	if (pixelbytes == 3) {
		fbaddress[startbyte] = RED(color,8);
		fbaddress[startbyte+1] = GREEN(color,8);
		fbaddress[startbyte+2] = BLUE(color,8);
	} else if (pixelbytes == 4) {
		*((__u32 *)(fbaddress+startbyte)) = color;
	} else {
		int compcolor;
		/* 5-bit, 5-bits, 5-bits */
		compcolor = RED(color,5) << 10 \
			| GREEN(color,5) << 5 \
			| BLUE(color,5);
		*((__u16 *)(fbaddress+startbyte)) = compcolor;
	}
}

/** Return pixel color */
static inline int getpixel(int x,int y)
{
	int startbyte = POINTPOS(x,y);
	int color;
	int result;

	if (pixelbytes == 3) {
		result = fbaddress[startbyte] << 16 \
			| fbaddress[startbyte+1] << 8 \
			| fbaddress[startbyte+2];
	} else if (pixelbytes == 4) {
		result = *((__u32 *)(fbaddress+startbyte)) & 0xffffff;
	} else {
		int red,green,blue;
		color = *((__u16 *)(fbaddress+startbyte));
		red = (color >> 10) & 0x1f;
		green = (color >> 5) & 0x1f;
		blue = color & 0x1f;
		result = (red << 16) | (green << 8) | blue;
	}
	return result;
}

static void clear_line(int y);
/** Scroll screen one row up */
static void scroll_screen(void)
{
	int i;

	for (i=0;i < (xres*yres*pixelbytes)-ROW_PIX; i++)
		fbaddress[i] = fbaddress[i+ROW_PIX];

	/* Clear last row */
	for (i=0; i < ROW_HEIGHT;i++)
		clear_line((rows-1)*ROW_HEIGHT+i);
	
}


/***************************************************************/
/* Screen specific function */

/** Fill line with color BGCOLOR */
static void clear_line(int y)
{
	int x;
	for (x=0; x<xres;x++)
		putpixel(x,y,BGCOLOR);
}

/** Fill screen with background color */
static void clear_screen(void)
{
	int y;

	for (y=0; y<yres;y++)
		clear_line(y);
}

static void invert_pixel(int x, int y)
{
	putpixel(x,y, ~getpixel(x,y));
}


/** Draw one line of glyph at a given position */
static void draw_glyph_line(int glline, int x, int y)
{
	int i;

	for (i=0; i < 8; i++)
		if (glline & (1 << (7-i))) {
			putpixel(x+i,y,FGCOLOR);
		} else
			putpixel(x+i,y,BGCOLOR);
}

/***************************************************************/
/* Character-console functions */

/** Draw character at given position */
static void draw_glyph(__u8 glyph, int col, int row)
{
	int y;

	for (y=0; y < FONT_SCANLINES; y++)
		draw_glyph_line(fb_font[glyph*FONT_SCANLINES+y],
				col*COL_WIDTH, row*ROW_HEIGHT+y);
}

/** Invert character at given position */
static void invert_char(int col, int row)
{
	int x,y;

	for (x=0; x < COL_WIDTH; x++)
		for (y=0; y < FONT_SCANLINES; y++)
			invert_pixel(col*COL_WIDTH+x,row*ROW_HEIGHT+y);
}

/** Draw character at default position */
static void draw_char(char chr)
{
	draw_glyph(chr, position % columns, position/columns);
}

/***************************************************************/
/* Stdout specific functions */

static void invert_cursor(void)
{
	invert_char(position % columns, position/columns);
}

/** Print character to screen
 *
 *  Emulate basic terminal commands
 */
static void fb_putchar(chardev_t *dev, char ch)
{
	spinlock_lock(&fb->lock);

	if (ch == '\n') {
		invert_cursor();
		position += columns;
		position -= position % columns;
	} else if (ch == '\r') {
		invert_cursor();
		position -= position % columns;
	} else if (ch == '\b') {
		invert_cursor();
		if (position % columns)
			position--;
	} else if (ch == '\t') {
		invert_cursor();
		do {
			draw_char(' ');
			position++;
		} while (position % 8);
	} else {
		draw_char(ch);
		position++;
	}
	if (position >= columns*rows) {
		position -= columns;
		scroll_screen();
	}
	invert_cursor();
	spinlock_unlock(&fb->lock);
}

static chardev_t framebuffer;
static chardev_operations_t fb_ops = {
	.write = fb_putchar,
};


/** Initialize framebuffer as a chardev output device
 *
 * @param addr Address of framebuffer
 * @param x X resolution
 * @param y Y resolution
 * @param bytes Bytes per pixel (2,3,4)
 */
void fb_init(__address addr, int x, int y, int bytes)
{
	fbaddress = (unsigned char *)addr;
	
	xres = x;
	yres = y;
	pixelbytes = bytes;
	
	rows = y/ROW_HEIGHT;
	columns = x/COL_WIDTH;

	clear_screen();
	invert_cursor();

	chardev_initialize("fb", &framebuffer, &fb_ops);
	stdout = &framebuffer;
}

