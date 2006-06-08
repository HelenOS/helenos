/*
 * Copyright (C) 2006 Jakub Vana
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

/** @defgroup fbs Framebuffers
 * @brief	HelenOS framebuffers.
 * @{ 
 * @}
 */
/** @defgroup fb Framebuffer
 * @brief	HelenOS framebuffer.
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
#include <async.h>

#include "font-8x16.h"
#include "fb.h"
#include "main.h"
#include "../console/screenbuffer.h"
#include "ppm.h"

#define DEFAULT_BGCOLOR                0xf0f0f0
#define DEFAULT_FGCOLOR                0x0

/***************************************************************/
/* Pixel specific fuctions */

typedef void (*conv2scr_fn_t)(void *, int);
typedef int (*conv2rgb_fn_t)(void *);

struct {
	__u8 *fbaddress ;

	unsigned int xres ;
	unsigned int yres ;
	unsigned int scanline ;
	unsigned int pixelbytes ;

	conv2scr_fn_t rgb2scr;
	conv2rgb_fn_t scr2rgb;
} screen;

typedef struct {
	int initialized;
	unsigned int x, y;
	unsigned int width, height;

	/* Text support in window */
	unsigned int rows, cols;
	/* Style for text printing */
	style_t style;
	/* Auto-cursor position */
	int cursor_active, cur_col, cur_row;
	int cursor_shown;
} viewport_t;

#define MAX_ANIM_LEN    8
#define MAX_ANIMATIONS  4
typedef struct {
	int initialized;
	int enabled;
	unsigned int vp;

	unsigned int pos;
	unsigned int animlen;
	unsigned int pixmaps[MAX_ANIM_LEN];
} animation_t;
static animation_t animations[MAX_ANIMATIONS];
static int anims_enabled;

/** Maximum number of saved pixmaps 
 * Pixmap is a saved rectangle
 */
#define MAX_PIXMAPS        256
typedef struct {
	unsigned int width;
	unsigned int height;
	__u8 *data;
} pixmap_t;
static pixmap_t pixmaps[MAX_PIXMAPS];

/* Viewport is a rectangular area on the screen */
#define MAX_VIEWPORTS 128
static viewport_t viewports[128];

/* Allow only 1 connection */
static int client_connected = 0;

#define RED(x, bits)	((x >> (16 + 8 - bits)) & ((1 << bits) - 1))
#define GREEN(x, bits)	((x >> (8 + 8 - bits)) & ((1 << bits) - 1))
#define BLUE(x, bits)	((x >> (8 - bits)) & ((1 << bits) - 1))

#define COL_WIDTH	8
#define ROW_BYTES	(screen.scanline * FONT_SCANLINES)

#define POINTPOS(x, y)	((y) * screen.scanline + (x) * screen.pixelbytes)

/* Conversion routines between different color representations */
static void rgb_4byte(void *dst, int rgb)
{
	*(int *)dst = rgb;
}

static int byte4_rgb(void *src)
{
	return (*(int *)src) & 0xffffff;
}

static void rgb_3byte(void *dst, int rgb)
{
	__u8 *scr = dst;
#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	scr[0] = RED(rgb, 8);
	scr[1] = GREEN(rgb, 8);
	scr[2] = BLUE(rgb, 8);
#else
	scr[2] = RED(rgb, 8);
	scr[1] = GREEN(rgb, 8);
	scr[0] = BLUE(rgb, 8);
#endif


}

static int byte3_rgb(void *src)
{
	__u8 *scr = src;
#if (defined(BIG_ENDIAN) || defined(FB_BIG_ENDIAN))
	return scr[0] << 16 | scr[1] << 8 | scr[2];
#else
	return scr[2] << 16 | scr[1] << 8 | scr[0];
#endif	
}

/**  16-bit depth (5:6:5) */
static void rgb_2byte(void *dst, int rgb)
{
	/* 5-bit, 6-bits, 5-bits */ 
	*((__u16 *)(dst)) = RED(rgb, 5) << 11 | GREEN(rgb, 6) << 5 | BLUE(rgb, 5);
}

/** 16-bit depth (5:6:5) */
static int byte2_rgb(void *src)
{
	int color = *(__u16 *)(src);
	return (((color >> 11) & 0x1f) << (16 + 3)) | (((color >> 5) & 0x3f) << (8 + 2)) | ((color & 0x1f) << 3);
}

/** Put pixel - 8-bit depth (3:2:3) */
static void rgb_1byte(void *dst, int rgb)
{
	*(__u8 *)dst = RED(rgb, 3) << 5 | GREEN(rgb, 2) << 3 | BLUE(rgb, 3);
}

/** Return pixel color - 8-bit depth (3:2:3) */
static int byte1_rgb(void *src)
{
	int color = *(__u8 *)src;
	return (((color >> 5) & 0x7) << (16 + 5)) | (((color >> 3) & 0x3) << (8 + 6)) | ((color & 0x7) << 5);
}

/** Put pixel into viewport 
 *
 * @param vp Viewport identification
 * @param x X coord relative to viewport
 * @param y Y coord relative to viewport
 * @param color RGB color 
 */
static void putpixel(int vp, unsigned int x, unsigned int y, int color)
{
	int dx = viewports[vp].x + x;
	int dy = viewports[vp].y + y;
	(*screen.rgb2scr)(&screen.fbaddress[POINTPOS(dx,dy)],color);
}
/** Get pixel from viewport */
static int getpixel(int vp, unsigned int x, unsigned int y)
{
	int dx = viewports[vp].x + x;
	int dy = viewports[vp].y + y;

	return (*screen.scr2rgb)(&screen.fbaddress[POINTPOS(dx,dy)]);
}

static inline void putpixel_mem(char *mem, unsigned int x, unsigned int y, 
				int color)
{
	(*screen.rgb2scr)(&mem[POINTPOS(x,y)],color);
}

static void draw_rectangle(int vp, unsigned int sx, unsigned int sy,
			   unsigned int width, unsigned int height,
			   int color)
{
	unsigned int x, y;
	static void *tmpline;

	if (!tmpline)
		tmpline = malloc(screen.scanline*screen.pixelbytes);

	/* Clear first line */
	for (x = 0; x < width; x++)
		putpixel_mem(tmpline, x, 0, color);

	/* Recompute to screen coords */
	sx += viewports[vp].x;
	sy += viewports[vp].y;
	/* Copy the rest */
	for (y = sy;y < sy+height; y++)
		memcpy(&screen.fbaddress[POINTPOS(sx,y)], tmpline, 
		       screen.pixelbytes * width);

}

/** Fill viewport with background color */
static void clear_port(int vp)
{
	viewport_t *vport = &viewports[vp];

	draw_rectangle(vp, 0, 0, vport->width, vport->height, vport->style.bg_color);
}

/** Scroll port up/down 
 *
 * @param vp Viewport to scroll
 * @param rows Positive number - scroll up, negative - scroll down
 */
static void scroll_port(int vp, int rows)
{
	int y;
	int startline;
	int endline;
	viewport_t *vport = &viewports[vp];
	
	if (rows > 0) {
		for (y=vport->y; y < vport->y+vport->height - rows*FONT_SCANLINES; y++)
			memcpy(&screen.fbaddress[POINTPOS(vport->x,y)],
			       &screen.fbaddress[POINTPOS(vport->x,y + rows*FONT_SCANLINES)],
			       screen.pixelbytes * vport->width);
		draw_rectangle(vp, 0, FONT_SCANLINES*(vport->rows - 1),
			       vport->width, FONT_SCANLINES, vport->style.bg_color);
	} else if (rows < 0) {
		rows = -rows;
		for (y=vport->y + vport->height-1; y >= vport->y + rows*FONT_SCANLINES; y--)
			memcpy(&screen.fbaddress[POINTPOS(vport->x,y)],
				&screen.fbaddress[POINTPOS(vport->x,y - rows*FONT_SCANLINES)],
				screen.pixelbytes * vport->width);
		draw_rectangle(vp, 0, 0, vport->width, FONT_SCANLINES, vport->style.bg_color);
	}
}

static void invert_pixel(int vp,unsigned int x, unsigned int y)
{
	putpixel(vp, x, y, ~getpixel(vp, x, y));
}


/***************************************************************/
/* Character-console functions */

/** Draw character at given position
 *
 * @param vp Viewport where the character is printed
 * @param sx Coordinates of top-left of the character
 * @param sy Coordinates of top-left of the character
 * @param style Color of the character
 * @param transparent If false, print background color
 */
static void draw_glyph(int vp,__u8 glyph, unsigned int sx, unsigned int sy, 
		       style_t style, int transparent)
{
	int i;
	unsigned int y;
	unsigned int glline;

	for (y = 0; y < FONT_SCANLINES; y++) {
		glline = fb_font[glyph * FONT_SCANLINES + y];
		for (i = 0; i < 8; i++) {
			if (glline & (1 << (7 - i)))
				putpixel(vp, sx + i, sy + y, style.fg_color);
			else if (!transparent)
				putpixel(vp, sx + i, sy + y, style.bg_color);
		}
	}
}

/** Invert character at given position */
static void invert_char(int vp,unsigned int row, unsigned int col)
{
	unsigned int x;
	unsigned int y;

	for (x = 0; x < COL_WIDTH; x++)
		for (y = 0; y < FONT_SCANLINES; y++)
			invert_pixel(vp, col * COL_WIDTH + x, row * FONT_SCANLINES + y);
}

/***************************************************************/
/* Stdout specific functions */


/** Create new viewport
 *
 * @return New viewport number
 */
static int viewport_create(unsigned int x, unsigned int y,unsigned int width, 
			   unsigned int height)
{
	int i;

	for (i=0; i < MAX_VIEWPORTS; i++) {
		if (!viewports[i].initialized)
			break;
	}
	if (i == MAX_VIEWPORTS)
		return ELIMIT;

	if (width ==0 || height == 0 ||
	    x+width > screen.xres || y+height > screen.yres)
		return EINVAL;
	if (width < FONT_SCANLINES || height < COL_WIDTH)
		return EINVAL;

	viewports[i].x = x;
	viewports[i].y = y;
	viewports[i].width = width;
	viewports[i].height = height;
	
	viewports[i].rows = height / FONT_SCANLINES;
	viewports[i].cols = width / COL_WIDTH;

	viewports[i].style.bg_color = DEFAULT_BGCOLOR;
	viewports[i].style.fg_color = DEFAULT_FGCOLOR;
	
	viewports[i].cur_col = 0;
	viewports[i].cur_row = 0;
	viewports[i].cursor_active = 0;

	viewports[i].initialized = 1;

	return i;
}


/** Initialize framebuffer as a chardev output device
 *
 * @param addr Address of theframebuffer
 * @param x    Screen width in pixels
 * @param y    Screen height in pixels
 * @param bpp  Bits per pixel (8, 16, 24, 32)
 * @param scan Bytes per one scanline
 *
 */
static void screen_init(void *addr, unsigned int xres, unsigned int yres, unsigned int bpp, unsigned int scan)
{
	switch (bpp) {
		case 8:
			screen.rgb2scr = rgb_1byte;
			screen.scr2rgb = byte1_rgb;
			screen.pixelbytes = 1;
			break;
		case 16:
			screen.rgb2scr = rgb_2byte;
			screen.scr2rgb = byte2_rgb;
			screen.pixelbytes = 2;
			break;
		case 24:
			screen.rgb2scr = rgb_3byte;
			screen.scr2rgb = byte3_rgb;
			screen.pixelbytes = 3;
			break;
		case 32:
			screen.rgb2scr = rgb_4byte;
			screen.scr2rgb = byte4_rgb;
			screen.pixelbytes = 4;
			break;
	}

		
	screen.fbaddress = (unsigned char *) addr;
	screen.xres = xres;
	screen.yres = yres;
	screen.scanline = scan;
	
	/* Create first viewport */
	viewport_create(0,0,xres,yres);
}

/** Hide cursor if it is shown */
static void cursor_hide(int vp)
{
	viewport_t *vport = &viewports[vp];

	if (vport->cursor_active && vport->cursor_shown) {
		invert_char(vp, vport->cur_row, vport->cur_col);
		vport->cursor_shown = 0;
	}
}

/** Show cursor if cursor showing is enabled */
static void cursor_print(int vp)
{
	viewport_t *vport = &viewports[vp];

	/* Do not check for cursor_shown */
	if (vport->cursor_active) {
		invert_char(vp, vport->cur_row, vport->cur_col);
		vport->cursor_shown = 1;
	}
}

/** Invert cursor, if it is enabled */
static void cursor_blink(int vp)
{
	viewport_t *vport = &viewports[vp];

	if (vport->cursor_shown)
		cursor_hide(vp);
	else
		cursor_print(vp);
}

/** Draw character at given position relative to viewport 
 * 
 * @param vp Viewport identification
 * @param c Character to print
 * @param row Screen position relative to viewport
 * @param col Screen position relative to viewport
 * @param transparent If false, print background color with character
 */
static void draw_char(int vp, char c, unsigned int row, unsigned int col, style_t style, int transparent)
{
	viewport_t *vport = &viewports[vp];

	/* Optimize - do not hide cursor if we are going to overwrite it */
	if (vport->cursor_active && vport->cursor_shown && 
	    (vport->cur_col != col || vport->cur_row != row))
		invert_char(vp, vport->cur_row, vport->cur_col);
	
	draw_glyph(vp, c, col * COL_WIDTH, row * FONT_SCANLINES, style, transparent);

	vport->cur_col = col;
	vport->cur_row = row;

	vport->cur_col++;
	if (vport->cur_col>= vport->cols) {
		vport->cur_col = 0;
		vport->cur_row++;
		if (vport->cur_row >= vport->rows)
			vport->cur_row--;
	}
	cursor_print(vp);
}

/** Draw text data to viewport
 *
 * @param vp Viewport id
 * @param data Text data fitting exactly into viewport
 */
static void draw_text_data(int vp, keyfield_t *data)
{
	viewport_t *vport = &viewports[vp];
	int i;
	char c;
	int col,row;

	clear_port(vp);
	for (i=0; i < vport->cols * vport->rows; i++) {
		if (data[i].character == ' ' && style_same(data[i].style,vport->style))
			continue;
		col = i % vport->cols;
		row = i / vport->cols;
		draw_glyph(vp, data[i].character, col * COL_WIDTH, row * FONT_SCANLINES, 
			   data[i].style, style_same(data[i].style,vport->style));
	}
	cursor_print(vp);
}


/** Return first free pixmap */
static int find_free_pixmap(void)
{
	int i;
	
	for (i=0;i < MAX_PIXMAPS;i++)
		if (!pixmaps[i].data)
			return i;
	return -1;
}

static void putpixel_pixmap(int pm, unsigned int x, unsigned int y, int color)
{
	pixmap_t *pmap = &pixmaps[pm];
	int pos = (y * pmap->width + x) * screen.pixelbytes;

	(*screen.rgb2scr)(&pmap->data[pos],color);
}

/** Create a new pixmap and return appropriate ID */
static int shm2pixmap(char *shm, size_t size)
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

	ppm_draw(shm, size, 0, 0, pmap->width, pmap->height, 
		 putpixel_pixmap, pm);

	return pm;
}

/** Handle shared memory communication calls
 *
 * Protocol for drawing pixmaps:
 * - FB_PREPARE_SHM(client shm identification)
 * - IPC_M_SEND_AS_AREA
 * - FB_DRAW_PPM(startx,starty)
 * - FB_DROP_SHM
 *
 * Protocol for text drawing
 * - IPC_M_SEND_AS_AREA
 * - FB_DRAW_TEXT_DATA
 *
 * @param callid Callid of the current call
 * @param call Current call data
 * @param vp Active viewport
 * @return 0 if the call was not handled byt this function, 1 otherwise
 *
 * note: this function is not threads safe, you would have
 * to redefine static variables with __thread
 */
static int shm_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	static keyfield_t *interbuffer = NULL;
	static size_t intersize = 0;

	static char *shm = NULL;
	static ipcarg_t shm_id = 0;
	static size_t shm_size;

	int handled = 1;
	int retval = 0;
	viewport_t *vport = &viewports[vp];
	unsigned int x,y;

	switch (IPC_GET_METHOD(*call)) {
	case IPC_M_AS_AREA_SEND:
		/* We accept one area for data interchange */
		if (IPC_GET_ARG1(*call) == shm_id) {
			void *dest = as_get_mappable_page(IPC_GET_ARG2(*call));
			shm_size = IPC_GET_ARG2(*call);
			if (!ipc_answer_fast(callid, 0, (sysarg_t)dest, 0)) 
				shm = dest;
			else
				shm_id = 0;
			if (shm[0] != 'P')
				while (1)
					;
			return 1;
		} else {
			intersize = IPC_GET_ARG2(*call);
			receive_comm_area(callid,call,(void **)&interbuffer);
		}
		return 1;
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
		if (x > vport->width || y > vport->height) {
			retval = EINVAL;
			break;
		}
		
		ppm_draw(shm, shm_size, IPC_GET_ARG1(*call), IPC_GET_ARG2(*call),
			 vport->width - x, vport->height - y, putpixel, vp);
		break;
	case FB_DRAW_TEXT_DATA:
		if (!interbuffer) {
			retval = EINVAL;
			break;
		}
		if (intersize < vport->cols*vport->rows*sizeof(*interbuffer)) {
			retval = EINVAL;
			break;
		}
		draw_text_data(vp, interbuffer);
		break;
	default:
		handled = 0;
	}
	
	if (handled)
		ipc_answer_fast(callid, retval, 0, 0);
	return handled;
}

/** Save viewport to pixmap */
static int save_vp_to_pixmap(int vp)
{
	int pm;
	pixmap_t *pmap;
	viewport_t *vport = &viewports[vp];
	int x,y;
	int rowsize;
	int tmp;

	pm = find_free_pixmap();
	if (pm == -1)
		return ELIMIT;
	
	pmap = &pixmaps[pm];
	pmap->data = malloc(screen.pixelbytes * vport->width * vport->height);
	if (!pmap->data)
		return ENOMEM;

	pmap->width = vport->width;
	pmap->height = vport->height;
	
	rowsize = vport->width * screen.pixelbytes;
	for (y=0;y < vport->height; y++) {
		tmp = (vport->y + y) * screen.scanline + vport->x * screen.pixelbytes;
		memcpy(pmap->data + rowsize*y, screen.fbaddress + tmp, rowsize); 
	}
	return pm;
}

/** Draw pixmap on screen
 *
 * @param vp Viewport to draw on
 * @param pm Pixmap identifier
 */
static int draw_pixmap(int vp, int pm)
{
	pixmap_t *pmap = &pixmaps[pm];
	viewport_t *vport = &viewports[vp];
	int x,y;
	int tmp, srcrowsize;
	int realwidth, realheight, realrowsize;

	if (!pmap->data)
		return EINVAL;

	realwidth = pmap->width <= vport->width ? pmap->width : vport->width;
	realheight = pmap->height <= vport->height ? pmap->height : vport->height;

	srcrowsize = vport->width * screen.pixelbytes;
	realrowsize = realwidth * screen.pixelbytes;

	for (y=0; y < realheight; y++) {
		tmp = (vport->y + y) * screen.scanline + vport->x * screen.pixelbytes;
		memcpy(screen.fbaddress + tmp, pmap->data + y * srcrowsize, realrowsize);
	}
	return 0;
}

/** Tick animation one step forward */
static void anims_tick(void)
{
	int i;
	static int counts = 0;
	
	/* Limit redrawing */
	counts = (counts+1) % 8;
	if (counts)
		return;

	for (i=0; i < MAX_ANIMATIONS; i++) {
		if (!animations[i].animlen || !animations[i].initialized || !animations[i].enabled)
			continue;
		draw_pixmap(animations[i].vp, animations[i].pixmaps[animations[i].pos]);
		animations[i].pos = (animations[i].pos+1) % animations[i].animlen;
	}
}

static int anim_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	int handled = 1;
	int retval = 0;
	int i,nvp;
	int newval;

	switch (IPC_GET_METHOD(*call)) {
	case FB_ANIM_CREATE:
		nvp = IPC_GET_ARG1(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp >= MAX_VIEWPORTS || nvp < 0 || !viewports[nvp].initialized) {
			retval = EINVAL;
			break;
		}
		for (i=0; i < MAX_ANIMATIONS; i++) {
			if (! animations[i].initialized)
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
		if (nvp >= MAX_ANIMATIONS || i < 0) {
			retval = EINVAL;
			break;
		}
		animations[i].initialized = 0;
		break;
	case FB_ANIM_ADDPIXMAP:
		i = IPC_GET_ARG1(*call);
		if (i >= MAX_ANIMATIONS || i < 0 || !animations[i].initialized) {
			retval = EINVAL;
			break;
		}
		if (animations[i].animlen == MAX_ANIM_LEN) {
			retval = ELIMIT;
			break;
		}
		newval = IPC_GET_ARG2(*call);
		if (newval < 0 || newval > MAX_PIXMAPS || !pixmaps[newval].data) {
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
		if (nvp >= MAX_VIEWPORTS || nvp < 0 || !viewports[nvp].initialized) {
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
		ipc_answer_fast(callid, retval, 0, 0);
	return handled;
}

/** Handler for messages concerning pixmap handling */
static int pixmap_handle(ipc_callid_t callid, ipc_call_t *call, int vp)
{
	int handled = 1;
	int retval = 0;
	int i,nvp;

	switch (IPC_GET_METHOD(*call)) {
	case FB_VP_DRAW_PIXMAP:
		nvp = IPC_GET_ARG1(*call);
		if (nvp == -1)
			nvp = vp;
		if (nvp < 0 || nvp >= MAX_VIEWPORTS || !viewports[nvp].initialized) {
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
		if (nvp < 0 || nvp >= MAX_VIEWPORTS || !viewports[nvp].initialized)
			retval = EINVAL;
		else
			retval = save_vp_to_pixmap(nvp);
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
		ipc_answer_fast(callid, retval, 0, 0);
	return handled;
	
}

/** Function for handling connections to FB
 *
 */
static void fb_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;
	int i;
	unsigned int row,col;
	char c;

	int vp = 0;
	viewport_t *vport = &viewports[0];

	if (client_connected) {
		ipc_answer_fast(iid, ELIMIT, 0,0);
		return;
	}
	client_connected = 1;
	ipc_answer_fast(iid, 0, 0, 0); /* Accept connection */

	while (1) {
		if (vport->cursor_active || anims_enabled)
			callid = async_get_call_timeout(&call,250000);
		else
			callid = async_get_call(&call);

		if (!callid) {
			cursor_blink(vp);
			anims_tick();
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
			client_connected = 0;
			/* cleanup other viewports */
			for (i=1; i < MAX_VIEWPORTS; i++)
				vport->initialized = 0;
			return; /* Exit thread */

		case FB_PUTCHAR:
		case FB_TRANS_PUTCHAR:
			c = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			col = IPC_GET_ARG3(call);
			if (row >= vport->rows || col >= vport->cols) {
				retval = EINVAL;
				break;
			}
			ipc_answer_fast(callid,0,0,0);

			draw_char(vp, c, row, col, vport->style, IPC_GET_METHOD(call) == FB_TRANS_PUTCHAR);
			continue; /* msg already answered */
		case FB_CLEAR:
			clear_port(vp);
			cursor_print(vp);
			retval = 0;
			break;
 		case FB_CURSOR_GOTO:
			row = IPC_GET_ARG1(call);
			col = IPC_GET_ARG2(call);
			if (row >= vport->rows || col >= vport->cols) {
				retval = EINVAL;
				break;
			}
 			retval = 0;
			cursor_hide(vp);
			vport->cur_col = col;
			vport->cur_row = row;
			cursor_print(vp);
 			break;
		case FB_CURSOR_VISIBILITY:
			cursor_hide(vp);
			vport->cursor_active = IPC_GET_ARG1(call);
			cursor_print(vp);
			retval = 0;
			break;
		case FB_GET_CSIZE:
			ipc_answer_fast(callid, 0, vport->rows, vport->cols);
			continue;
		case FB_SCROLL:
			i = IPC_GET_ARG1(call);
			if (i > vport->rows || i < (- (int)vport->rows)) {
				retval = EINVAL;
				break;
			}
			cursor_hide(vp);
			scroll_port(vp, i);
			cursor_print(vp);
			retval = 0;
			break;
		case FB_VIEWPORT_SWITCH:
			i = IPC_GET_ARG1(call);
			if (i < 0 || i >= MAX_VIEWPORTS) {
				retval = EINVAL;
				break;
			}
			if (! viewports[i].initialized ) {
				retval = EADDRNOTAVAIL;
				break;
			}
			cursor_hide(vp);
			vp = i;
			vport = &viewports[vp];
			cursor_print(vp);
			retval = 0;
			break;
		case FB_VIEWPORT_CREATE:
			retval = viewport_create(IPC_GET_ARG1(call) >> 16,
						 IPC_GET_ARG1(call) & 0xffff,
						 IPC_GET_ARG2(call) >> 16,
						 IPC_GET_ARG2(call) & 0xffff);
			break;
		case FB_VIEWPORT_DELETE:
			i = IPC_GET_ARG1(call);
			if (i < 0 || i >= MAX_VIEWPORTS) {
				retval = EINVAL;
				break;
			}
			if (! viewports[i].initialized ) {
				retval = EADDRNOTAVAIL;
				break;
			}
			viewports[i].initialized = 0;
			retval = 0;
			break;
		case FB_SET_STYLE:
			vport->style.fg_color = IPC_GET_ARG1(call);
			vport->style.bg_color = IPC_GET_ARG2(call);
			retval = 0;
			break;
		case FB_GET_RESOLUTION:
			ipc_answer_fast(callid, 0, screen.xres,screen.yres);
			continue;
		default:
			retval = ENOENT;
		}
		ipc_answer_fast(callid,retval,0,0);
	}
}

/** Initialization of framebuffer */
int fb_init(void)
{
	void *fb_ph_addr;
	unsigned int fb_width;
	unsigned int fb_height;
	unsigned int fb_bpp;
	unsigned int fb_scanline;
	void *fb_addr;
	size_t asz;

	async_set_client_connection(fb_client_connection);

	fb_ph_addr=(void *)sysinfo_value("fb.address.physical");
	fb_width=sysinfo_value("fb.width");
	fb_height=sysinfo_value("fb.height");
	fb_bpp=sysinfo_value("fb.bpp");
	fb_scanline=sysinfo_value("fb.scanline");

	asz = fb_scanline*fb_height;
	fb_addr = as_get_mappable_page(asz);
	
	map_physmem(fb_ph_addr, fb_addr, ALIGN_UP(asz,PAGE_SIZE) >>PAGE_WIDTH,
		    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE);
	
	screen_init(fb_addr, fb_width, fb_height, fb_bpp, fb_scanline);

	return 0;
}


/** 
 * @}
 */
