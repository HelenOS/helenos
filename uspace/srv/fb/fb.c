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

/**
 * @defgroup fb Graphical framebuffer
 * @brief HelenOS graphical framebuffer.
 * @ingroup fbs
 * @{
 */

/** @file
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ddi.h>
#include <sysinfo.h>
#include <align.h>
#include <as.h>
#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <kernel/errno.h>
#include <kernel/genarch/fb/visuals.h>
#include <io/color.h>
#include <io/style.h>
#include <async.h>
#include <fibril.h>
#include <bool.h>
#include <stdio.h>
#include <byteorder.h>

#include "font-8x16.h"
#include "fb.h"
#include "main.h"
#include "../console/screenbuffer.h"
#include "ppm.h"

#include "pointer.xbm"
#include "pointer_mask.xbm"

#define DEFAULT_BGCOLOR  0xf0f0f0
#define DEFAULT_FGCOLOR  0x000000

#define GLYPH_UNAVAIL    '?'

#define MAX_ANIM_LEN     8
#define MAX_ANIMATIONS   4
#define MAX_PIXMAPS      256  /**< Maximum number of saved pixmaps */
#define MAX_VIEWPORTS    128  /**< Viewport is a rectangular area on the screen */

/** Function to render a pixel from a RGB value. */
typedef void (*rgb_conv_t)(void *, uint32_t);

/** Function to render a bit mask. */
typedef void (*mask_conv_t)(void *, bool);

/** Function to draw a glyph. */
typedef void (*dg_t)(unsigned int x, unsigned int y, bool cursor,
    uint8_t *glyphs, uint32_t glyph, uint32_t fg_color, uint32_t bg_color);

struct {
	uint8_t *fb_addr;
	
	unsigned int xres;
	unsigned int yres;
	
	unsigned int scanline;
	unsigned int glyphscanline;
	
	unsigned int pixelbytes;
	unsigned int glyphbytes;
	
	/** Pre-rendered mask for rendering glyphs. Specific for the visual. */
	uint8_t *glyphs;
	
	rgb_conv_t rgb_conv;
	mask_conv_t mask_conv;
} screen;

/** Backbuffer character cell. */
typedef struct {
	uint32_t glyph;
	uint32_t fg_color;
	uint32_t bg_color;
} bb_cell_t;

typedef struct {
	bool initialized;
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	
	/* Text support in window */
	unsigned int cols;
	unsigned int rows;
	
	/*
	 * Style and glyphs for text printing
	 */
	
	/** Current attributes. */
	attr_rgb_t attr;
	
	uint8_t *bgpixel;
	
	/**
	 * Glyph drawing function for this viewport.  Different viewports
	 * might use different drawing functions depending on whether their
	 * scanlines are aligned on a word boundary.
	 */
	dg_t dglyph;
	
	/* Auto-cursor position */
	bool cursor_active;
	unsigned int cur_col;
	unsigned int cur_row;
	bool cursor_shown;
	
	/* Back buffer */
	bb_cell_t *backbuf;
	unsigned int bbsize;
} viewport_t;

typedef struct {
	bool initialized;
	bool enabled;
	unsigned int vp;
	
	unsigned int pos;
	unsigned int animlen;
	unsigned int pixmaps[MAX_ANIM_LEN];
} animation_t;

static animation_t animations[MAX_ANIMATIONS];
static bool anims_enabled;

typedef struct {
	unsigned int width;
	unsigned int height;
	uint8_t *data;
} pixmap_t;

static pixmap_t pixmaps[MAX_PIXMAPS];
static viewport_t viewports[128];

static bool client_connected = false;  /**< Allow only 1 connection */

static uint32_t color_table[16] = {
	[COLOR_BLACK]       = 0x000000,
	[COLOR_BLUE]        = 0x0000f0,
	[COLOR_GREEN]       = 0x00f000,
	[COLOR_CYAN]        = 0x00f0f0,
	[COLOR_RED]         = 0xf00000,
	[COLOR_MAGENTA]     = 0xf000f0,
	[COLOR_YELLOW]      = 0xf0f000,
	[COLOR_WHITE]       = 0xf0f0f0,
	
	[8 + COLOR_BLACK]   = 0x000000,
	[8 + COLOR_BLUE]    = 0x0000ff,
	[8 + COLOR_GREEN]   = 0x00ff00,
	[8 + COLOR_CYAN]    = 0x00ffff,
	[8 + COLOR_RED]     = 0xff0000,
	[8 + COLOR_MAGENTA] = 0xff00ff,
	[8 + COLOR_YELLOW]  = 0xffff00,
	[8 + COLOR_WHITE]   = 0xffffff,
};

static int rgb_from_attr(attr_rgb_t *rgb, const attrs_t *a);
static int rgb_from_style(attr_rgb_t *rgb, int style);
static int rgb_from_idx(attr_rgb_t *rgb, ipcarg_t fg_color,
    ipcarg_t bg_color, ipcarg_t flags);

static int fb_set_color(viewport_t *vport, ipcarg_t fg_color,
    ipcarg_t bg_color, ipcarg_t attr);

static void draw_glyph_aligned(unsigned int x, unsigned int y, bool cursor,
    uint8_t *glyphs, uint32_t glyph, uint32_t fg_color, uint32_t bg_color);
static void draw_glyph_fallback(unsigned int x, unsigned int y, bool cursor,
    uint8_t *glyphs, uint32_t glyph, uint32_t fg_color, uint32_t bg_color);

static void draw_vp_glyph(viewport_t *vport, bool cursor, unsigned int col,
    unsigned int row);


#define RED(x, bits)                 (((x) >> (8 + 8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define GREEN(x, bits)               (((x) >> (8 + 8 - (bits))) & ((1 << (bits)) - 1))
#define BLUE(x, bits)                (((x) >> (8 - (bits))) & ((1 << (bits)) - 1))

#define COL2X(col)                   ((col) * FONT_WIDTH)
#define ROW2Y(row)                   ((row) * FONT_SCANLINES)

#define X2COL(x)                     ((x) / FONT_WIDTH)
#define Y2ROW(y)                     ((y) / FONT_SCANLINES)

#define FB_POS(x, y)                 ((y) * screen.scanline + (x) * screen.pixelbytes)
#define BB_POS(vport, col, row)      ((row) * vport->cols + (col))
#define GLYPH_POS(glyph, y, cursor)  (((glyph) + (cursor) * FONT_GLYPHS) * screen.glyphbytes + (y) * screen.glyphscanline)

/*
 * RGB conversion and mask functions.
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

static void mask_0888(void *dst, bool mask)
{
	bgr_0888(dst, mask ? 0xffffff : 0);
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

static void mask_8880(void *dst, bool mask)
{
	bgr_8880(dst, mask ? 0xffffff : 0);
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

static void mask_888(void *dst, bool mask)
{
	bgr_888(dst, mask ? 0xffffff : 0);
}

static void rgb_555_be(void *dst, uint32_t rgb)
{
	*((uint16_t *) dst) = host2uint16_t_be(RED(rgb, 5) << 10 |
	    GREEN(rgb, 5) << 5 | BLUE(rgb, 5));
}

static void rgb_555_le(void *dst, uint32_t rgb)
{
	*((uint16_t *) dst) = host2uint16_t_le(RED(rgb, 5) << 10 |
	    GREEN(rgb, 5) << 5 | BLUE(rgb, 5));
}

static void rgb_565_be(void *dst, uint32_t rgb)
{
	*((uint16_t *) dst) = host2uint16_t_be(RED(rgb, 5) << 11 |
	    GREEN(rgb, 6) << 5 | BLUE(rgb, 5));
}

static void rgb_565_le(void *dst, uint32_t rgb)
{
	*((uint16_t *) dst) = host2uint16_t_le(RED(rgb, 5) << 11 |
	    GREEN(rgb, 6) << 5 | BLUE(rgb, 5));
}

static void mask_555(void *dst, bool mask)
{
	rgb_555_be(dst, mask ? 0xffffff : 0);
}

static void mask_565(void *dst, bool mask)
{
	rgb_565_be(dst, mask ? 0xffffff : 0);
}

static void bgr_323(void *dst, uint32_t rgb)
{
	*((uint8_t *) dst)
	    = ~((RED(rgb, 3) << 5) | (GREEN(rgb, 2) << 3) | BLUE(rgb, 3));
}

static void mask_323(void *dst, bool mask)
{
	bgr_323(dst, mask ? 0x0 : ~0x0);
}

/** Draw a filled rectangle.
 *
 * @note Need real implementation that does not access VRAM twice.
 *
 */
static void draw_filled_rect(unsigned int x0, unsigned int y0, unsigned int x1,
    unsigned int y1, uint32_t color)
{
	unsigned int x;
	unsigned int y;
	unsigned int copy_bytes;
	
	uint8_t *sp;
	uint8_t *dp;
	uint8_t cbuf[4];
	
	if ((y0 >= y1) || (x0 >= x1))
		return;
	
	screen.rgb_conv(cbuf, color);
	
	sp = &screen.fb_addr[FB_POS(x0, y0)];
	dp = sp;
	
	/* Draw the first line. */
	for (x = x0; x < x1; x++) {
		memcpy(dp, cbuf, screen.pixelbytes);
		dp += screen.pixelbytes;
	}
	
	dp = sp + screen.scanline;
	copy_bytes = (x1 - x0) * screen.pixelbytes;
	
	/* Draw the remaining lines by copying. */
	for (y = y0 + 1; y < y1; y++) {
		memcpy(dp, sp, copy_bytes);
		dp += screen.scanline;
	}
}

/** Redraw viewport.
 *
 * @param vport Viewport to redraw
 *
 */
static void vport_redraw(viewport_t *vport)
{
	unsigned int col;
	unsigned int row;
	
	for (row = 0; row < vport->rows; row++) {
		for (col = 0; col < vport->cols; col++) {
			draw_vp_glyph(vport, false, col, row);
		}
	}
	
	if (COL2X(vport->cols) < vport->width) {
		draw_filled_rect(
		    vport->x + COL2X(vport->cols), vport->y,
		    vport->x + vport->width, vport->y + vport->height,
		    vport->attr.bg_color);
	}
	
	if (ROW2Y(vport->rows) < vport->height) {
		draw_filled_rect(
		    vport->x, vport->y + ROW2Y(vport->rows),
		    vport->x + vport->width, vport->y + vport->height,
		    vport->attr.bg_color);
	}
}

static void backbuf_clear(bb_cell_t *backbuf, size_t len, uint32_t fg_color,
    uint32_t bg_color)
{
	size_t i;
	
	for (i = 0; i < len; i++) {
		backbuf[i].glyph = 0;
		backbuf[i].fg_color = fg_color;
		backbuf[i].bg_color = bg_color;
	}
}

/** Clear viewport.
 *
 * @param vport Viewport to clear
 *
 */
static void vport_clear(viewport_t *vport)
{
	backbuf_clear(vport->backbuf, vport->cols * vport->rows,
	    vport->attr.fg_color, vport->attr.bg_color);
	vport_redraw(vport);
}

/** Scroll viewport by the specified number of lines.
 *
 * @param vport Viewport to scroll
 * @param lines Number of lines to scroll
 *
 */
static void vport_scroll(viewport_t *vport, int lines)
{
	unsigned int col;
	unsigned int row;
	unsigned int x;
	unsigned int y;
	uint32_t glyph;
	uint32_t fg_color;
	uint32_t bg_color;
	bb_cell_t *bbp;
	bb_cell_t *xbp;
	
	/*
	 * Redraw.
	 */
	
	y = vport->y;
	for (row = 0; row < vport->rows; row++) {
		x = vport->x;
		for (col = 0; col < vport->cols; col++) {
			if (((int) row + lines >= 0) &&
			    ((int) row + lines < (int) vport->rows)) {
				xbp = &vport->backbuf[BB_POS(vport, col, row + lines)];
				bbp = &vport->backbuf[BB_POS(vport, col, row)];
				
				glyph = xbp->glyph;
				fg_color = xbp->fg_color;
				bg_color = xbp->bg_color;
				
				if ((bbp->glyph == glyph)
				   && (bbp->fg_color == xbp->fg_color)
				   && (bbp->bg_color == xbp->bg_color)) {
					x += FONT_WIDTH;
					continue;
				}
			} else {
				glyph = 0;
				fg_color = vport->attr.fg_color;
				bg_color = vport->attr.bg_color;
			}
			
			(*vport->dglyph)(x, y, false, screen.glyphs, glyph,
			    fg_color, bg_color);
			x += FONT_WIDTH;
		}
		y += FONT_SCANLINES;
	}
	
	/*
	 * Scroll backbuffer.
	 */
	
	if (lines > 0) {
		memmove(vport->backbuf, vport->backbuf + vport->cols * lines,
		    vport->cols * (vport->rows - lines) * sizeof(bb_cell_t));
		backbuf_clear(&vport->backbuf[BB_POS(vport, 0, vport->rows - lines)],
		    vport->cols * lines, vport->attr.fg_color, vport->attr.bg_color);
	} else {
		memmove(vport->backbuf - vport->cols * lines, vport->backbuf,
		    vport->cols * (vport->rows + lines) * sizeof(bb_cell_t));
		backbuf_clear(vport->backbuf, - vport->cols * lines,
		    vport->attr.fg_color, vport->attr.bg_color);
	}
}

/** Render glyphs
 *
 * Convert glyphs from device independent font
 * description to current visual representation.
 *
 */
static void render_glyphs(void)
{
	unsigned int glyph;
	
	for (glyph = 0; glyph < FONT_GLYPHS; glyph++) {
		unsigned int y;
		
		for (y = 0; y < FONT_SCANLINES; y++) {
			unsigned int x;
			
			for (x = 0; x < FONT_WIDTH; x++) {
				screen.mask_conv(&screen.glyphs[GLYPH_POS(glyph, y, false) + x * screen.pixelbytes],
				    (fb_font[glyph][y] & (1 << (7 - x))) ? true : false);
				
				screen.mask_conv(&screen.glyphs[GLYPH_POS(glyph, y, true) + x * screen.pixelbytes],
				    (fb_font[glyph][y] & (1 << (7 - x))) ? false : true);
			}
		}
	}
}

/** Create new viewport
 *
 * @param x      Origin of the viewport (x).
 * @param y      Origin of the viewport (y).
 * @param width  Width of the viewport.
 * @param height Height of the viewport.
 *
 * @return New viewport number.
 *
 */
static int vport_create(unsigned int x, unsigned int y,
    unsigned int width, unsigned int height)
{
	unsigned int i;
	
	for (i = 0; i < MAX_VIEWPORTS; i++) {
		if (!viewports[i].initialized)
			break;
	}
	
	if (i == MAX_VIEWPORTS)
		return ELIMIT;
	
	unsigned int cols = width / FONT_WIDTH;
	unsigned int rows = height / FONT_SCANLINES;
	unsigned int bbsize = cols * rows * sizeof(bb_cell_t);
	unsigned int word_size = sizeof(unsigned long);
	
	bb_cell_t *backbuf = (bb_cell_t *) malloc(bbsize);
	if (!backbuf)
		return ENOMEM;
	
	uint8_t *bgpixel = (uint8_t *) malloc(screen.pixelbytes);
	if (!bgpixel) {
		free(backbuf);
		return ENOMEM;
	}
	
	backbuf_clear(backbuf, cols * rows, DEFAULT_FGCOLOR, DEFAULT_BGCOLOR);
	memset(bgpixel, 0, screen.pixelbytes);
	
	viewports[i].x = x;
	viewports[i].y = y;
	viewports[i].width = width;
	viewports[i].height = height;
	
	viewports[i].cols = cols;
	viewports[i].rows = rows;
	
	viewports[i].attr.bg_color = DEFAULT_BGCOLOR;
	viewports[i].attr.fg_color = DEFAULT_FGCOLOR;
	
	viewports[i].bgpixel = bgpixel;
	
	/*
	 * Conditions necessary to select aligned version:
	 *  - word size is divisible by pixelbytes
	 *  - cell scanline size is divisible by word size
	 *  - cell scanlines are word-aligned
	 *
	 */
	if (((word_size % screen.pixelbytes) == 0)
	    && ((FONT_WIDTH * screen.pixelbytes) % word_size == 0)
	    && ((x * screen.pixelbytes) % word_size == 0)
	    && (screen.scanline % word_size == 0)) {
		viewports[i].dglyph = draw_glyph_aligned;
	} else {
		viewports[i].dglyph = draw_glyph_fallback;
	}
	
	viewports[i].cur_col = 0;
	viewports[i].cur_row = 0;
	viewports[i].cursor_active = false;
	viewports[i].cursor_shown = false;
	
	viewports[i].bbsize = bbsize;
	viewports[i].backbuf = backbuf;
	
	viewports[i].initialized = true;
	
	screen.rgb_conv(viewports[i].bgpixel, viewports[i].attr.bg_color);
	
	return i;
}


/** Initialize framebuffer as a chardev output device
 *
 * @param addr   Address of the framebuffer
 * @param xres   Screen width in pixels
 * @param yres   Screen height in pixels
 * @param visual Bits per pixel (8, 16, 24, 32)
 * @param scan   Bytes per one scanline
 *
 */
static bool screen_init(void *addr, unsigned int xres, unsigned int yres,
    unsigned int scan, unsigned int visual)
{
	switch (visual) {
	case VISUAL_INDIRECT_8:
		screen.rgb_conv = bgr_323;
		screen.mask_conv = mask_323;
		screen.pixelbytes = 1;
		break;
	case VISUAL_RGB_5_5_5_LE:
		screen.rgb_conv = rgb_555_le;
		screen.mask_conv = mask_555;
		screen.pixelbytes = 2;
		break;
	case VISUAL_RGB_5_5_5_BE:
		screen.rgb_conv = rgb_555_be;
		screen.mask_conv = mask_555;
		screen.pixelbytes = 2;
		break;
	case VISUAL_RGB_5_6_5_LE:
		screen.rgb_conv = rgb_565_le;
		screen.mask_conv = mask_565;
		screen.pixelbytes = 2;
		break;
	case VISUAL_RGB_5_6_5_BE:
		screen.rgb_conv = rgb_565_be;
		screen.mask_conv = mask_565;
		screen.pixelbytes = 2;
		break;
	case VISUAL_RGB_8_8_8:
		screen.rgb_conv = rgb_888;
		screen.mask_conv = mask_888;
		screen.pixelbytes = 3;
		break;
	case VISUAL_BGR_8_8_8:
		screen.rgb_conv = bgr_888;
		screen.mask_conv = mask_888;
		screen.pixelbytes = 3;
		break;
	case VISUAL_RGB_8_8_8_0:
		screen.rgb_conv = rgb_8880;
		screen.mask_conv = mask_8880;
		screen.pixelbytes = 4;
		break;
	case VISUAL_RGB_0_8_8_8:
		screen.rgb_conv = rgb_0888;
		screen.mask_conv = mask_0888;
		screen.pixelbytes = 4;
		break;
	case VISUAL_BGR_0_8_8_8:
		screen.rgb_conv = bgr_0888;
		screen.mask_conv = mask_0888;
		screen.pixelbytes = 4;
		break;
	case VISUAL_BGR_8_8_8_0:
		screen.rgb_conv = bgr_8880;
		screen.mask_conv = mask_8880;
		screen.pixelbytes = 4;
		break;
	default:
		return false;
	}
	
	screen.fb_addr = (unsigned char *) addr;
	screen.xres = xres;
	screen.yres = yres;
	screen.scanline = scan;
	
	screen.glyphscanline = FONT_WIDTH * screen.pixelbytes;
	screen.glyphbytes = screen.glyphscanline * FONT_SCANLINES;
	
	size_t glyphsize = 2 * FONT_GLYPHS * screen.glyphbytes;
	uint8_t *glyphs = (uint8_t *) malloc(glyphsize);
	if (!glyphs)
		return false;
	
	memset(glyphs, 0, glyphsize);
	screen.glyphs = glyphs;
	
	render_glyphs();
	
	/* Create first viewport */
	vport_create(0, 0, xres, yres);
	
	return true;
}


/** Draw a glyph, takes advantage of alignment.
 *
 * This version can only be used if the following conditions are met:
 *
 *   - word size is divisible by pixelbytes
 *   - cell scanline size is divisible by word size
 *   - cell scanlines are word-aligned
 *
 * It makes use of the pre-rendered mask to process (possibly) several
 * pixels at once (word size / pixelbytes pixels at a time are processed)
 * making it very fast. Most notably this version is not applicable at 24 bits
 * per pixel.
 *
 * @param x        x coordinate of top-left corner on screen.
 * @param y        y coordinate of top-left corner on screen.
 * @param cursor   Draw glyph with cursor
 * @param glyphs   Pointer to font bitmap.
 * @param glyph    Code of the glyph to draw.
 * @param fg_color Foreground color.
 * @param bg_color Backgroudn color.
 *
 */
static void draw_glyph_aligned(unsigned int x, unsigned int y, bool cursor,
    uint8_t *glyphs, uint32_t glyph, uint32_t fg_color, uint32_t bg_color)
{
	unsigned int i;
	unsigned int yd;
	unsigned long fg_buf;
	unsigned long bg_buf;
	unsigned long mask;
	
	/*
	 * Prepare a pair of words, one filled with foreground-color
	 * pattern and the other filled with background-color pattern.
	 */
	for (i = 0; i < sizeof(unsigned long) / screen.pixelbytes; i++) {
		screen.rgb_conv(&((uint8_t *) &fg_buf)[i * screen.pixelbytes],
		    fg_color);
		screen.rgb_conv(&((uint8_t *) &bg_buf)[i * screen.pixelbytes],
		    bg_color);
	}
	
	/* Pointer to the current position in the mask. */
	unsigned long *maskp = (unsigned long *) &glyphs[GLYPH_POS(glyph, 0, cursor)];
	
	/* Pointer to the current position on the screen. */
	unsigned long *dp = (unsigned long *) &screen.fb_addr[FB_POS(x, y)];
	
	/* Width of the character cell in words. */
	unsigned int ww = FONT_WIDTH * screen.pixelbytes / sizeof(unsigned long);
	
	/* Offset to add when moving to another screen scanline. */
	unsigned int d_add = screen.scanline - FONT_WIDTH * screen.pixelbytes;
	
	for (yd = 0; yd < FONT_SCANLINES; yd++) {
		/*
		 * Now process the cell scanline, combining foreground
		 * and background color patters using the pre-rendered mask.
		 */
		for (i = 0; i < ww; i++) {
			mask = *maskp++;
			*dp++ = (fg_buf & mask) | (bg_buf & ~mask);
		}
		
		/* Move to the beginning of the next scanline of the cell. */
		dp = (unsigned long *) ((uint8_t *) dp + d_add);
	}
}

/** Draw a glyph, fallback version.
 *
 * This version does not make use of the pre-rendered mask, it uses
 * the font bitmap directly. It works always, but it is slower.
 *
 * @param x        x coordinate of top-left corner on screen.
 * @param y        y coordinate of top-left corner on screen.
 * @param cursor   Draw glyph with cursor
 * @param glyphs   Pointer to font bitmap.
 * @param glyph    Code of the glyph to draw.
 * @param fg_color Foreground color.
 * @param bg_color Backgroudn color.
 *
 */
void draw_glyph_fallback(unsigned int x, unsigned int y, bool cursor,
    uint8_t *glyphs, uint32_t glyph, uint32_t fg_color, uint32_t bg_color)
{
	unsigned int i;
	unsigned int j;
	unsigned int yd;
	uint8_t fg_buf[4];
	uint8_t bg_buf[4];
	uint8_t *sp;
	uint8_t b;
	
	/* Pre-render 1x the foreground and background color pixels. */
	if (cursor) {
		screen.rgb_conv(fg_buf, bg_color);
		screen.rgb_conv(bg_buf, fg_color);
	} else {
		screen.rgb_conv(fg_buf, fg_color);
		screen.rgb_conv(bg_buf, bg_color);
	}
	
	/* Pointer to the current position on the screen. */
	uint8_t *dp = (uint8_t *) &screen.fb_addr[FB_POS(x, y)];
	
	/* Offset to add when moving to another screen scanline. */
	unsigned int d_add = screen.scanline - FONT_WIDTH * screen.pixelbytes;
	
	for (yd = 0; yd < FONT_SCANLINES; yd++) {
		/* Byte containing bits of the glyph scanline. */
		b = fb_font[glyph][yd];
		
		for (i = 0; i < FONT_WIDTH; i++) {
			/* Choose color based on the current bit. */
			sp = (b & 0x80) ? fg_buf : bg_buf;
			
			/* Copy the pixel. */
			for (j = 0; j < screen.pixelbytes; j++) {
				*dp++ = *sp++;
			}
			
			/* Move to the next bit. */
			b = b << 1;
		}
		
		/* Move to the beginning of the next scanline of the cell. */
		dp += d_add;
	}
}

/** Draw glyph at specified position in viewport.
 *
 * @param vport  Viewport identification
 * @param cursor Draw glyph with cursor
 * @param col    Screen position relative to viewport
 * @param row    Screen position relative to viewport
 *
 */
static void draw_vp_glyph(viewport_t *vport, bool cursor, unsigned int col,
    unsigned int row)
{
	unsigned int x = vport->x + COL2X(col);
	unsigned int y = vport->y + ROW2Y(row);
	
	uint32_t glyph = vport->backbuf[BB_POS(vport, col, row)].glyph;
	uint32_t fg_color = vport->backbuf[BB_POS(vport, col, row)].fg_color;
	uint32_t bg_color = vport->backbuf[BB_POS(vport, col, row)].bg_color;
	
	(*vport->dglyph)(x, y, cursor, screen.glyphs, glyph,
	    fg_color, bg_color);
}

/** Hide cursor if it is shown
 *
 */
static void cursor_hide(viewport_t *vport)
{
	if ((vport->cursor_active) && (vport->cursor_shown)) {
		draw_vp_glyph(vport, false, vport->cur_col, vport->cur_row);
		vport->cursor_shown = false;
	}
}


/** Show cursor if cursor showing is enabled
 *
 */
static void cursor_show(viewport_t *vport)
{
	/* Do not check for cursor_shown */
	if (vport->cursor_active) {
		draw_vp_glyph(vport, true, vport->cur_col, vport->cur_row);
		vport->cursor_shown = true;
	}
}


/** Invert cursor, if it is enabled
 *
 */
static void cursor_blink(viewport_t *vport)
{
	if (vport->cursor_shown)
		cursor_hide(vport);
	else
		cursor_show(vport);
}


/** Draw character at given position relative to viewport
 *
 * @param vport  Viewport identification
 * @param c      Character to draw
 * @param col    Screen position relative to viewport
 * @param row    Screen position relative to viewport
 *
 */
static void draw_char(viewport_t *vport, wchar_t c, unsigned int col, unsigned int row)
{
	bb_cell_t *bbp;
	
	/* Do not hide cursor if we are going to overwrite it */
	if ((vport->cursor_active) && (vport->cursor_shown) &&
	    ((vport->cur_col != col) || (vport->cur_row != row)))
		cursor_hide(vport);
	
	bbp = &vport->backbuf[BB_POS(vport, col, row)];
	bbp->glyph = fb_font_glyph(c);
	bbp->fg_color = vport->attr.fg_color;
	bbp->bg_color = vport->attr.bg_color;
	
	draw_vp_glyph(vport, false, col, row);
	
	vport->cur_col = col;
	vport->cur_row = row;
	
	vport->cur_col++;
	if (vport->cur_col >= vport->cols) {
		vport->cur_col = 0;
		vport->cur_row++;
		if (vport->cur_row >= vport->rows)
			vport->cur_row--;
	}
	
	cursor_show(vport);
}

/** Draw text data to viewport.
 *
 * @param vport Viewport id
 * @param data  Text data.
 * @param x     Leftmost column of the area.
 * @param y     Topmost row of the area.
 * @param w     Number of rows.
 * @param h     Number of columns.
 *
 */
static void draw_text_data(viewport_t *vport, keyfield_t *data, unsigned int x,
    unsigned int y, unsigned int w, unsigned int h)
{
	unsigned int i;
	unsigned int j;
	bb_cell_t *bbp;
	attrs_t *a;
	attr_rgb_t rgb;
	
	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			unsigned int col = x + i;
			unsigned int row = y + j;
			
			bbp = &vport->backbuf[BB_POS(vport, col, row)];
			
			a = &data[j * w + i].attrs;
			rgb_from_attr(&rgb, a);
			
			bbp->glyph = fb_font_glyph(data[j * w + i].character);
			bbp->fg_color = rgb.fg_color;
			bbp->bg_color = rgb.bg_color;
			
			draw_vp_glyph(vport, false, col, row);
		}
	}
	cursor_show(vport);
}


static void putpixel_pixmap(void *data, unsigned int x, unsigned int y, uint32_t color)
{
	int pm = *((int *) data);
	pixmap_t *pmap = &pixmaps[pm];
	unsigned int pos = (y * pmap->width + x) * screen.pixelbytes;
	
	screen.rgb_conv(&pmap->data[pos], color);
}


static void putpixel(void *data, unsigned int x, unsigned int y, uint32_t color)
{
	viewport_t *vport = (viewport_t *) data;
	unsigned int dx = vport->x + x;
	unsigned int dy = vport->y + y;
	
	screen.rgb_conv(&screen.fb_addr[FB_POS(dx, dy)], color);
}


/** Return first free pixmap
 *
 */
static int find_free_pixmap(void)
{
	unsigned int i;
	
	for (i = 0; i < MAX_PIXMAPS; i++)
		if (!pixmaps[i].data)
			return i;
	
	return -1;
}


/** Create a new pixmap and return appropriate ID
 *
 */
static int shm2pixmap(unsigned char *shm, size_t size)
{
	int pm;
	pixmap_t *pmap;
	
	pm = find_free_pixmap();
	if (pm == -1)
		return ELIMIT;
	
	pmap = &pixmaps[pm];
	
	if (ppm_get_data(shm, size, &pmap->width, &pmap->height))
		return EINVAL;
	
	pmap->data = malloc(pmap->width * pmap->height * screen.pixelbytes);
	if (!pmap->data)
		return ENOMEM;
	
	ppm_draw(shm, size, 0, 0, pmap->width, pmap->height, putpixel_pixmap, (void *) &pm);
	
	return pm;
}


/** Handle shared memory communication calls
 *
 * Protocol for drawing pixmaps:
 * - FB_PREPARE_SHM(client shm identification)
 * - IPC_M_AS_AREA_SEND
 * - FB_DRAW_PPM(startx, starty)
 * - FB_DROP_SHM
 *
 * Protocol for text drawing
 * - IPC_M_AS_AREA_SEND
 * - FB_DRAW_TEXT_DATA
 *
 * @param callid Callid of the current call
 * @param call   Current call data
 * @param vp     Active viewport
 *
 * @return false if the call was not handled byt this function, true otherwise
 *
 * Note: this function is not thread-safe, you would have
 * to redefine static variables with fibril_local.
 *
 */
static bool shm_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	static keyfield_t *interbuffer = NULL;
	static size_t intersize = 0;
	
	static unsigned char *shm = NULL;
	static ipcarg_t shm_id = 0;
	static size_t shm_size;
	
	bool handled = true;
	int retval = EOK;
	viewport_t *vport = &viewports[vp];
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
	
	switch (IPC_GET_METHOD(*call)) {
	case IPC_M_SHARE_OUT:
		/* We accept one area for data interchange */
		if (IPC_GET_ARG1(*call) == shm_id) {
			void *dest = as_get_mappable_page(IPC_GET_ARG2(*call));
			shm_size = IPC_GET_ARG2(*call);
			if (ipc_answer_1(callid, EOK, (sysarg_t) dest)) {
				shm_id = 0;
				return false;
			}
			shm = dest;
			
			if (shm[0] != 'P')
				return false;
			
			return true;
		} else {
			intersize = IPC_GET_ARG2(*call);
			receive_comm_area(callid, call, (void *) &interbuffer);
		}
		return true;
	case FB_PREPARE_SHM:
		if (shm_id)
			retval = EBUSY;
		else 
			shm_id = IPC_GET_ARG1(*call);
		break;
		
	case FB_DROP_SHM:
		if (shm) {
			as_area_destroy(shm);
			shm = NULL;
		}
		shm_id = 0;
		break;
		
	case FB_SHM2PIXMAP:
		if (!shm) {
			retval = EINVAL;
			break;
		}
		retval = shm2pixmap(shm, shm_size);
		break;
	case FB_DRAW_PPM:
		if (!shm) {
			retval = EINVAL;
			break;
		}
		x = IPC_GET_ARG1(*call);
		y = IPC_GET_ARG2(*call);
		
		if ((x > vport->width) || (y > vport->height)) {
			retval = EINVAL;
			break;
		}
		
		ppm_draw(shm, shm_size, IPC_GET_ARG1(*call),
		    IPC_GET_ARG2(*call), vport->width - x, vport->height - y, putpixel, (void *) vport);
		break;
	case FB_DRAW_TEXT_DATA:
		x = IPC_GET_ARG1(*call);
		y = IPC_GET_ARG2(*call);
		w = IPC_GET_ARG3(*call);
		h = IPC_GET_ARG4(*call);
		if (!interbuffer) {
			retval = EINVAL;
			break;
		}
		if (x + w > vport->cols || y + h > vport->rows) {
			retval = EINVAL;
			break;
		}
		if (intersize < w * h * sizeof(*interbuffer)) {
			retval = EINVAL;
			break;
		}
		draw_text_data(vport, interbuffer, x, y, w, h);
		break;
	default:
		handled = false;
	}
	
	if (handled)
		ipc_answer_0(callid, retval);
	return handled;
}


static void copy_vp_to_pixmap(viewport_t *vport, pixmap_t *pmap)
{
	unsigned int width = vport->width;
	unsigned int height = vport->height;
	
	if (width + vport->x > screen.xres)
		width = screen.xres - vport->x;
	if (height + vport->y > screen.yres)
		height = screen.yres - vport->y;
	
	unsigned int realwidth = pmap->width <= width ? pmap->width : width;
	unsigned int realheight = pmap->height <= height ? pmap->height : height;
	
	unsigned int srcrowsize = vport->width * screen.pixelbytes;
	unsigned int realrowsize = realwidth * screen.pixelbytes;
	
	unsigned int y;
	for (y = 0; y < realheight; y++) {
		unsigned int tmp = (vport->y + y) * screen.scanline + vport->x * screen.pixelbytes;
		memcpy(pmap->data + srcrowsize * y, screen.fb_addr + tmp, realrowsize);
	}
}


/** Save viewport to pixmap
 *
 */
static int save_vp_to_pixmap(viewport_t *vport)
{
	int pm;
	pixmap_t *pmap;
	
	pm = find_free_pixmap();
	if (pm == -1)
		return ELIMIT;
	
	pmap = &pixmaps[pm];
	pmap->data = malloc(screen.pixelbytes * vport->width * vport->height);
	if (!pmap->data)
		return ENOMEM;
	
	pmap->width = vport->width;
	pmap->height = vport->height;
	
	copy_vp_to_pixmap(vport, pmap);
	
	return pm;
}


/** Draw pixmap on screen
 *
 * @param vp Viewport to draw on
 * @param pm Pixmap identifier
 *
 */
static int draw_pixmap(int vp, int pm)
{
	pixmap_t *pmap = &pixmaps[pm];
	viewport_t *vport = &viewports[vp];
	
	unsigned int width = vport->width;
	unsigned int height = vport->height;
	
	if (width + vport->x > screen.xres)
		width = screen.xres - vport->x;
	if (height + vport->y > screen.yres)
		height = screen.yres - vport->y;
	
	if (!pmap->data)
		return EINVAL;
	
	unsigned int realwidth = pmap->width <= width ? pmap->width : width;
	unsigned int realheight = pmap->height <= height ? pmap->height : height;
	
	unsigned int srcrowsize = vport->width * screen.pixelbytes;
	unsigned int realrowsize = realwidth * screen.pixelbytes;
	
	unsigned int y;
	for (y = 0; y < realheight; y++) {
		unsigned int tmp = (vport->y + y) * screen.scanline + vport->x * screen.pixelbytes;
		memcpy(screen.fb_addr + tmp, pmap->data + y * srcrowsize, realrowsize);
	}
	
	return EOK;
}


/** Tick animation one step forward
 *
 */
static void anims_tick(void)
{
	unsigned int i;
	static int counts = 0;
	
	/* Limit redrawing */
	counts = (counts + 1) % 8;
	if (counts)
		return;
	
	for (i = 0; i < MAX_ANIMATIONS; i++) {
		if ((!animations[i].animlen) || (!animations[i].initialized) ||
		    (!animations[i].enabled))
			continue;
		
		draw_pixmap(animations[i].vp, animations[i].pixmaps[animations[i].pos]);
		animations[i].pos = (animations[i].pos + 1) % animations[i].animlen;
	}
}


static unsigned int pointer_x;
static unsigned int pointer_y;
static bool pointer_shown, pointer_enabled;
static int pointer_vport = -1;
static int pointer_pixmap = -1;


static void mouse_show(void)
{
	int i, j;
	int visibility;
	int color;
	int bytepos;
	
	if ((pointer_shown) || (!pointer_enabled))
		return;
	
	/* Save image under the pointer. */
	if (pointer_vport == -1) {
		pointer_vport = vport_create(pointer_x, pointer_y, pointer_width, pointer_height);
		if (pointer_vport < 0)
			return;
	} else {
		viewports[pointer_vport].x = pointer_x;
		viewports[pointer_vport].y = pointer_y;
	}
	
	if (pointer_pixmap == -1)
		pointer_pixmap = save_vp_to_pixmap(&viewports[pointer_vport]);
	else
		copy_vp_to_pixmap(&viewports[pointer_vport], &pixmaps[pointer_pixmap]);
	
	/* Draw mouse pointer. */
	for (i = 0; i < pointer_height; i++)
		for (j = 0; j < pointer_width; j++) {
			bytepos = i * ((pointer_width - 1) / 8 + 1) + j / 8;
			visibility = pointer_mask_bits[bytepos] &
			    (1 << (j % 8));
			if (visibility) {
				color = pointer_bits[bytepos] &
				    (1 << (j % 8)) ? 0 : 0xffffff;
				if (pointer_x + j < screen.xres && pointer_y +
				    i < screen.yres)
					putpixel(&viewports[0], pointer_x + j,
					    pointer_y + i, color);
			}
		}
	pointer_shown = 1;
}


static void mouse_hide(void)
{
	/* Restore image under the pointer. */
	if (pointer_shown) {
		draw_pixmap(pointer_vport, pointer_pixmap);
		pointer_shown = 0;
	}
}


static void mouse_move(unsigned int x, unsigned int y)
{
	mouse_hide();
	pointer_x = x;
	pointer_y = y;
	mouse_show();
}


static int anim_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	bool handled = true;
	int retval = EOK;
	int i, nvp;
	int newval;
	
	switch (IPC_GET_METHOD(*call)) {
	case FB_ANIM_CREATE:
		nvp = IPC_GET_ARG1(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp >= MAX_VIEWPORTS || nvp < 0 ||
			!viewports[nvp].initialized) {
			retval = EINVAL;
			break;
		}
		for (i = 0; i < MAX_ANIMATIONS; i++) {
			if (!animations[i].initialized)
				break;
		}
		if (i == MAX_ANIMATIONS) {
			retval = ELIMIT;
			break;
		}
		animations[i].initialized = 1;
		animations[i].animlen = 0;
		animations[i].pos = 0;
		animations[i].enabled = 0;
		animations[i].vp = nvp;
		retval = i;
		break;
	case FB_ANIM_DROP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_ANIMATIONS || i < 0) {
			retval = EINVAL;
			break;
		}
		animations[i].initialized = 0;
		break;
	case FB_ANIM_ADDPIXMAP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_ANIMATIONS || i < 0 ||
			!animations[i].initialized) {
			retval = EINVAL;
			break;
		}
		if (animations[i].animlen == MAX_ANIM_LEN) {
			retval = ELIMIT;
			break;
		}
		newval = IPC_GET_ARG2(*call);
		if (newval < 0 || newval > MAX_PIXMAPS ||
			!pixmaps[newval].data) {
			retval = EINVAL;
			break;
		}
		animations[i].pixmaps[animations[i].animlen++] = newval;
		break;
	case FB_ANIM_CHGVP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_ANIMATIONS || i < 0) {
			retval = EINVAL;
			break;
		}
		nvp = IPC_GET_ARG2(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp >= MAX_VIEWPORTS || nvp < 0 ||
			!viewports[nvp].initialized) {
			retval = EINVAL;
			break;
		}
		animations[i].vp = nvp;
		break;
	case FB_ANIM_START:
	case FB_ANIM_STOP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_ANIMATIONS || i < 0) {
			retval = EINVAL;
			break;
		}
		newval = (IPC_GET_METHOD(*call) == FB_ANIM_START);
		if (newval ^ animations[i].enabled) {
			animations[i].enabled = newval;
			anims_enabled += newval ? 1 : -1;
		}
		break;
	default:
		handled = 0;
	}
	if (handled)
		ipc_answer_0(callid, retval);
	return handled;
}


/** Handler for messages concerning pixmap handling
 *
 */
static int pixmap_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	bool handled = true;
	int retval = EOK;
	int i, nvp;
	
	switch (IPC_GET_METHOD(*call)) {
	case FB_VP_DRAW_PIXMAP:
		nvp = IPC_GET_ARG1(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp < 0 || nvp >= MAX_VIEWPORTS ||
			!viewports[nvp].initialized) {
			retval = EINVAL;
			break;
		}
		i = IPC_GET_ARG2(*call);
		retval = draw_pixmap(nvp, i);
		break;
	case FB_VP2PIXMAP:
		nvp = IPC_GET_ARG1(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp < 0 || nvp >= MAX_VIEWPORTS ||
			!viewports[nvp].initialized)
			retval = EINVAL;
		else
			retval = save_vp_to_pixmap(&viewports[nvp]);
		break;
	case FB_DROP_PIXMAP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_PIXMAPS) {
			retval = EINVAL;
			break;
		}
		if (pixmaps[i].data) {
			free(pixmaps[i].data);
			pixmaps[i].data = NULL;
		}
		break;
	default:
		handled = 0;
	}
	
	if (handled)
		ipc_answer_0(callid, retval);
	return handled;
	
}

static int rgb_from_style(attr_rgb_t *rgb, int style)
{
	switch (style) {
	case STYLE_NORMAL:
		rgb->fg_color = color_table[COLOR_BLACK];
		rgb->bg_color = color_table[COLOR_WHITE];
		break;
	case STYLE_EMPHASIS:
		rgb->fg_color = color_table[COLOR_RED];
		rgb->bg_color = color_table[COLOR_WHITE];
		break;
	default:
		return EINVAL;
	}

	return EOK;
}

static int rgb_from_idx(attr_rgb_t *rgb, ipcarg_t fg_color,
    ipcarg_t bg_color, ipcarg_t flags)
{
	fg_color = (fg_color & 7) | ((flags & CATTR_BRIGHT) ? 8 : 0);
	bg_color = (bg_color & 7) | ((flags & CATTR_BRIGHT) ? 8 : 0);

	rgb->fg_color = color_table[fg_color];
	rgb->bg_color = color_table[bg_color];

	return EOK;
}

static int rgb_from_attr(attr_rgb_t *rgb, const attrs_t *a)
{
	int rc;

	switch (a->t) {
	case at_style:
		rc = rgb_from_style(rgb, a->a.s.style);
		break;
	case at_idx:
		rc = rgb_from_idx(rgb, a->a.i.fg_color,
		    a->a.i.bg_color, a->a.i.flags);
		break;
	case at_rgb:
		*rgb = a->a.r;
		rc = EOK;
		break;
	}

	return rc;
}

static int fb_set_style(viewport_t *vport, ipcarg_t style)
{
	return rgb_from_style(&vport->attr, (int) style);
}

static int fb_set_color(viewport_t *vport, ipcarg_t fg_color,
    ipcarg_t bg_color, ipcarg_t flags)
{
	return rgb_from_idx(&vport->attr, fg_color, bg_color, flags);
}

/** Function for handling connections to FB
 *
 */
static void fb_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	unsigned int vp = 0;
	viewport_t *vport = &viewports[vp];
	
	if (client_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	
	/* Accept connection */
	client_connected = true;
	ipc_answer_0(iid, EOK);
	
	while (true) {
		ipc_callid_t callid;
		ipc_call_t call;
		int retval;
		unsigned int i;
		int scroll;
		wchar_t ch;
		unsigned int col, row;
		
		if ((vport->cursor_active) || (anims_enabled))
			callid = async_get_call_timeout(&call, 250000);
		else
			callid = async_get_call(&call);
		
		mouse_hide();
		if (!callid) {
			cursor_blink(vport);
			anims_tick();
			mouse_show();
			continue;
		}
		
		if (shm_handle(callid, &call, vp))
			continue;
		
		if (pixmap_handle(callid, &call, vp))
			continue;
		
		if (anim_handle(callid, &call, vp))
			continue;
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			client_connected = false;
			
			/* Cleanup other viewports */
			for (i = 1; i < MAX_VIEWPORTS; i++)
				vport->initialized = false;
			
			/* Exit thread */
			return;
		
		case FB_PUTCHAR:
			ch = IPC_GET_ARG1(call);
			col = IPC_GET_ARG2(call);
			row = IPC_GET_ARG3(call);
			
			if ((col >= vport->cols) || (row >= vport->rows)) {
				retval = EINVAL;
				break;
			}
			ipc_answer_0(callid, EOK);
			
			draw_char(vport, ch, col, row);
			
			/* Message already answered */
			continue;
		case FB_CLEAR:
			vport_clear(vport);
			cursor_show(vport);
			retval = EOK;
			break;
		case FB_CURSOR_GOTO:
			col = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			
			if ((col >= vport->cols) || (row >= vport->rows)) {
				retval = EINVAL;
				break;
			}
			retval = EOK;
			
			cursor_hide(vport);
			vport->cur_col = col;
			vport->cur_row = row;
			cursor_show(vport);
			break;
		case FB_CURSOR_VISIBILITY:
			cursor_hide(vport);
			vport->cursor_active = IPC_GET_ARG1(call);
			cursor_show(vport);
			retval = EOK;
			break;
		case FB_GET_CSIZE:
			ipc_answer_2(callid, EOK, vport->cols, vport->rows);
			continue;
		case FB_GET_COLOR_CAP:
			ipc_answer_1(callid, EOK, FB_CCAP_RGB);
			continue;
		case FB_SCROLL:
			scroll = IPC_GET_ARG1(call);
			if ((scroll > (int) vport->rows) || (scroll < (-(int) vport->rows))) {
				retval = EINVAL;
				break;
			}
			cursor_hide(vport);
			vport_scroll(vport, scroll);
			cursor_show(vport);
			retval = EOK;
			break;
		case FB_VIEWPORT_SWITCH:
			i = IPC_GET_ARG1(call);
			if (i >= MAX_VIEWPORTS) {
				retval = EINVAL;
				break;
			}
			if (!viewports[i].initialized) {
				retval = EADDRNOTAVAIL;
				break;
			}
			cursor_hide(vport);
			vp = i;
			vport = &viewports[vp];
			cursor_show(vport);
			retval = EOK;
			break;
		case FB_VIEWPORT_CREATE:
			retval = vport_create(IPC_GET_ARG1(call) >> 16,
			    IPC_GET_ARG1(call) & 0xffff,
			    IPC_GET_ARG2(call) >> 16,
			    IPC_GET_ARG2(call) & 0xffff);
			break;
		case FB_VIEWPORT_DELETE:
			i = IPC_GET_ARG1(call);
			if (i >= MAX_VIEWPORTS) {
				retval = EINVAL;
				break;
			}
			if (!viewports[i].initialized) {
				retval = EADDRNOTAVAIL;
				break;
			}
			viewports[i].initialized = false;
			if (viewports[i].bgpixel)
				free(viewports[i].bgpixel);
			if (viewports[i].backbuf)
				free(viewports[i].backbuf);
			retval = EOK;
			break;
		case FB_SET_STYLE:
			retval = fb_set_style(vport, IPC_GET_ARG1(call));
			break;
		case FB_SET_COLOR:
			retval = fb_set_color(vport, IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call), IPC_GET_ARG3(call));
			break;
		case FB_SET_RGB_COLOR:
			vport->attr.fg_color = IPC_GET_ARG1(call);
			vport->attr.bg_color = IPC_GET_ARG2(call);
			retval = EOK;
			break;
		case FB_GET_RESOLUTION:
			ipc_answer_2(callid, EOK, screen.xres, screen.yres);
			continue;
		case FB_POINTER_MOVE:
			pointer_enabled = true;
			mouse_move(IPC_GET_ARG1(call), IPC_GET_ARG2(call));
			retval = EOK;
			break;
		case FB_SCREEN_YIELD:
		case FB_SCREEN_RECLAIM:
			retval = EOK;
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
}

/** Initialization of framebuffer
 *
 */
int fb_init(void)
{
	async_set_client_connection(fb_client_connection);
	
	void *fb_ph_addr = (void *) sysinfo_value("fb.address.physical");
	unsigned int fb_offset = sysinfo_value("fb.offset");
	unsigned int fb_width = sysinfo_value("fb.width");
	unsigned int fb_height = sysinfo_value("fb.height");
	unsigned int fb_scanline = sysinfo_value("fb.scanline");
	unsigned int fb_visual = sysinfo_value("fb.visual");

	unsigned int fbsize = fb_scanline * fb_height;
	void *fb_addr = as_get_mappable_page(fbsize);

	if (physmem_map(fb_ph_addr + fb_offset, fb_addr,
	    ALIGN_UP(fbsize, PAGE_SIZE) >> PAGE_WIDTH, AS_AREA_READ | AS_AREA_WRITE) != 0)
		return -1;

	if (screen_init(fb_addr, fb_width, fb_height, fb_scanline, fb_visual))
		return 0;

	return -1;
}

/**
 * @}
 */
