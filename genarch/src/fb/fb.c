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

static unsigned char *fbaddress=NULL;
static int xres,yres;
static int position=0;
static int columns=0;
static int rows=0;

#define COL_WIDTH   8
#define ROW_HEIGHT  (FONT_SCANLINES)
#define ROW_PIX     (xres*ROW_HEIGHT*3)

#define BGCOLOR   0x000080
#define FGCOLOR   0xffff00

#define RED(x)    ((x >> 16) & 0xff)
#define GREEN(x)  ((x >> 8) & 0xff)
#define BLUE(x)   (x & 0xff)

#define POINTPOS(x,y,colidx)   ((y*xres+x)*3+colidx)

/** Draw pixel of given color on screen */
static inline void putpixel(int x,int y,int color)
{
	fbaddress[POINTPOS(x,y,0)] = RED(color);
	fbaddress[POINTPOS(x,y,1)] = GREEN(color);
	fbaddress[POINTPOS(x,y,2)] = BLUE(color);
}

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
	fbaddress[POINTPOS(x,y,0)] = ~fbaddress[POINTPOS(x,y,0)];
	fbaddress[POINTPOS(x,y,1)] = ~fbaddress[POINTPOS(x,y,1)];
	fbaddress[POINTPOS(x,y,2)] = ~fbaddress[POINTPOS(x,y,2)];
}

/** Scroll screen one row up */
static void scroll_screen(void)
{
	int i;

	for (i=0;i < (xres*yres*3)-ROW_PIX; i++)
		fbaddress[i] = fbaddress[i+ROW_PIX];

	/* Clear last row */
	for (i=0; i < ROW_HEIGHT;i++)
		clear_line((rows-1)*ROW_HEIGHT+i);
	
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

/** Draw character at given position */
static void draw_glyph(unsigned char glyph, int col, int row)
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
 */
void fb_init(__address addr, int x, int y)
{
	fbaddress = (unsigned char *)addr;
	
	xres = x;
	yres = y;
	
	rows = y/ROW_HEIGHT;
	columns = x/COL_WIDTH;

	clear_screen();
	invert_cursor();

	chardev_initialize("fb", &framebuffer, &fb_ops);
	stdout = &framebuffer;
}

