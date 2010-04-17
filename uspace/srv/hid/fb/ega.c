/*
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

/** @defgroup egafb EGA framebuffer
 * @brief	HelenOS EGA framebuffer.
 * @ingroup fbs
 * @{
 */ 
/** @file
 */

#include <stdlib.h>
#include <unistd.h>
#include <align.h>
#include <async.h>
#include <ipc/ipc.h>
#include <errno.h>
#include <stdio.h>
#include <ddi.h>
#include <sysinfo.h>
#include <as.h>
#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <libarch/ddi.h>
#include <io/style.h>
#include <io/color.h>
#include <sys/types.h>

#include "ega.h"
#include "../console/screenbuffer.h"
#include "main.h"

#define MAX_SAVED_SCREENS 256
typedef struct saved_screen {
	short *data;
} saved_screen;

saved_screen saved_screens[MAX_SAVED_SCREENS];

#define EGA_IO_BASE ((ioport8_t *) 0x3d4)
#define EGA_IO_SIZE 2

int ega_normal_color = 0x0f;
int ega_inverted_color = 0xf0;

#define NORMAL_COLOR		ega_normal_color
#define INVERTED_COLOR		ega_inverted_color

/* Allow only 1 connection */
static int client_connected = 0;

static unsigned int scr_width;
static unsigned int scr_height;
static uint8_t *scr_addr;

static unsigned int style;

static unsigned attr_to_ega_style(const attrs_t *a);
static uint8_t ega_glyph(wchar_t ch);

static void clrscr(void)
{
	unsigned i;
	
	for (i = 0; i < scr_width * scr_height; i++) {
		scr_addr[i * 2] = ' ';
		scr_addr[i * 2 + 1] = style;
	}
}

static void cursor_goto(unsigned int col, unsigned int row)
{
	int ega_cursor;

	ega_cursor = col + scr_width * row;
	
	pio_write_8(EGA_IO_BASE, 0xe);
	pio_write_8(EGA_IO_BASE + 1, (ega_cursor >> 8) & 0xff);
	pio_write_8(EGA_IO_BASE, 0xf);
	pio_write_8(EGA_IO_BASE + 1, ega_cursor & 0xff);
}

static void cursor_disable(void)
{
	uint8_t stat;

	pio_write_8(EGA_IO_BASE, 0xa);
	stat = pio_read_8(EGA_IO_BASE + 1);
	pio_write_8(EGA_IO_BASE, 0xa);
	pio_write_8(EGA_IO_BASE + 1, stat | (1 << 5));
}

static void cursor_enable(void)
{
	uint8_t stat;

	pio_write_8(EGA_IO_BASE, 0xa);
	stat = pio_read_8(EGA_IO_BASE + 1);
	pio_write_8(EGA_IO_BASE, 0xa);
	pio_write_8(EGA_IO_BASE + 1, stat & (~(1 << 5)));
}

static void scroll(int rows)
{
	unsigned i;

	if (rows > 0) {
		memmove(scr_addr, ((char *) scr_addr) + rows * scr_width * 2,
		    scr_width * scr_height * 2 - rows * scr_width * 2);
		for (i = 0; i < rows * scr_width; i++)
			(((short *) scr_addr) + scr_width * scr_height - rows *
			    scr_width)[i] = ((style << 8) + ' ');
	} else if (rows < 0) {
		memmove(((char *)scr_addr) - rows * scr_width * 2, scr_addr,
		    scr_width * scr_height * 2 + rows * scr_width * 2);
		for (i = 0; i < -rows * scr_width; i++)
			((short *)scr_addr)[i] = ((style << 8 ) + ' ');
	}
}

static void printchar(wchar_t c, unsigned int col, unsigned int row)
{
	scr_addr[(row * scr_width + col) * 2] = ega_glyph(c);
	scr_addr[(row * scr_width + col) * 2 + 1] = style;
	
	cursor_goto(col + 1, row);
}

/** Draw text data to viewport.
 *
 * @param vport Viewport id
 * @param data  Text data.
 * @param x	Leftmost column of the area.
 * @param y	Topmost row of the area.
 * @param w	Number of rows.
 * @param h	Number of columns.
 */
static void draw_text_data(keyfield_t *data, unsigned int x,
    unsigned int y, unsigned int w, unsigned int h)
{
	unsigned int i, j;
	keyfield_t *field;
	uint8_t *dp;

	for (j = 0; j < h; j++) {
		for (i = 0; i < w; i++) {
			field = &data[j * w + i];
			dp = &scr_addr[2 * ((y + j) * scr_width + (x + i))];

			dp[0] = ega_glyph(field->character);
			dp[1] = attr_to_ega_style(&field->attrs);
		}
	}
}

static int save_screen(void)
{
	int i;

	for (i = 0; (i < MAX_SAVED_SCREENS) && (saved_screens[i].data); i++)
		;
	if (i == MAX_SAVED_SCREENS) 
		return EINVAL;
	if (!(saved_screens[i].data = malloc(2 * scr_width * scr_height))) 
		return ENOMEM;
	memcpy(saved_screens[i].data, scr_addr, 2 * scr_width * scr_height);

	return i;
}

static int print_screen(int i)
{
	if (saved_screens[i].data)
		memcpy(scr_addr, saved_screens[i].data, 2 * scr_width *
		    scr_height);
	else
		return EINVAL;
	return i;
}

static int style_to_ega_style(int style)
{
	unsigned int ega_style;

	switch (style) {
	case STYLE_NORMAL:
		ega_style = INVERTED_COLOR;
		break;
	case STYLE_EMPHASIS:
		ega_style = INVERTED_COLOR | 4;
		break;
	default:
		return INVERTED_COLOR;
	}

	return ega_style;
}

static unsigned int color_to_ega_style(int fg_color, int bg_color, int attr)
{
	unsigned int style;

	style = (fg_color & 7) | ((bg_color & 7) << 4);
	if (attr & CATTR_BRIGHT)
		style = style | 0x08;

	return style;
}

static unsigned int rgb_to_ega_style(uint32_t fg, uint32_t bg)
{
	return (fg > bg) ? NORMAL_COLOR : INVERTED_COLOR;
}

static unsigned attr_to_ega_style(const attrs_t *a)
{
	switch (a->t) {
	case at_style:
		return style_to_ega_style(a->a.s.style);
	case at_rgb:
		return rgb_to_ega_style(a->a.r.fg_color, a->a.r.bg_color);
	case at_idx:
		return color_to_ega_style(a->a.i.fg_color,
		    a->a.i.bg_color, a->a.i.flags);
	default:
		return INVERTED_COLOR;
	}
}

static uint8_t ega_glyph(wchar_t ch)
{
	if (ch >= 0 && ch < 128)
		return ch;

	return '?';
}

static void ega_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	int retval;
	ipc_callid_t callid;
	ipc_call_t call;
	wchar_t c;
	unsigned int row, col, w, h;
	int bg_color, fg_color, attr;
	uint32_t bg_rgb, fg_rgb;
	keyfield_t *interbuf = NULL;
	size_t intersize = 0;
	int i;

	if (client_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	client_connected = 1;
	ipc_answer_0(iid, EOK); /* Accept connection */

	while (1) {
		callid = async_get_call(&call);
 		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			client_connected = 0;
			ipc_answer_0(callid, EOK);
			return; /* Exit thread */
		case IPC_M_SHARE_OUT:
			/* We accept one area for data interchange */
			intersize = IPC_GET_ARG2(call);
			if (intersize >= scr_width * scr_height *
			    sizeof(*interbuf)) {
				receive_comm_area(callid, &call,
				    (void *) &interbuf);
				continue;
			}
			retval = EINVAL;
			break;
		case FB_DRAW_TEXT_DATA:
			col = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			w = IPC_GET_ARG3(call);
			h = IPC_GET_ARG4(call);
			if (!interbuf) {
				retval = EINVAL;
				break;
			}
			if (col + w > scr_width || row + h > scr_height) {
				retval = EINVAL;
				break;
			}
			draw_text_data(interbuf, col, row, w, h);
			retval = 0;
			break;
		case FB_GET_CSIZE:
			ipc_answer_2(callid, EOK, scr_width, scr_height);
			continue;
		case FB_GET_COLOR_CAP:
			ipc_answer_1(callid, EOK, FB_CCAP_INDEXED);
			continue;
		case FB_CLEAR:
			clrscr();
			retval = 0;
			break;
		case FB_PUTCHAR:
			c = IPC_GET_ARG1(call);
			col = IPC_GET_ARG2(call);
			row = IPC_GET_ARG3(call);
			if (col >= scr_width || row >= scr_height) {
				retval = EINVAL;
				break;
			}
			printchar(c, col, row);
			retval = 0;
			break;
 		case FB_CURSOR_GOTO:
 			col = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			if (row >= scr_height || col >= scr_width) {
				retval = EINVAL;
				break;
			}
			cursor_goto(col, row);
 			retval = 0;
 			break;
		case FB_SCROLL:
			i = IPC_GET_ARG1(call);
			if (i > (int) scr_height || i < -((int) scr_height)) {
				retval = EINVAL;
				break;
			}
			scroll(i);
			retval = 0;
			break;
		case FB_CURSOR_VISIBILITY:
			if (IPC_GET_ARG1(call))
				cursor_enable();
			else
				cursor_disable();
			retval = 0;
			break;
		case FB_SET_STYLE:
			style = style_to_ega_style(IPC_GET_ARG1(call));
			retval = 0;
			break;
		case FB_SET_COLOR:
			fg_color = IPC_GET_ARG1(call);
			bg_color = IPC_GET_ARG2(call);
			attr = IPC_GET_ARG3(call);
			style = color_to_ega_style(fg_color, bg_color, attr);
			retval = 0;
			break;
		case FB_SET_RGB_COLOR:
			fg_rgb = IPC_GET_ARG1(call);
			bg_rgb = IPC_GET_ARG2(call);
			style = rgb_to_ega_style(fg_rgb, bg_rgb);
			retval = 0;
			break;
		case FB_VP_DRAW_PIXMAP:
			i = IPC_GET_ARG2(call);
			retval = print_screen(i);
			break;
		case FB_VP2PIXMAP:
			retval = save_screen();
			break;
		case FB_DROP_PIXMAP:
			i = IPC_GET_ARG1(call);
			if (i >= MAX_SAVED_SCREENS) {
				retval = EINVAL;
				break;
			}
			if (saved_screens[i].data) {
				free(saved_screens[i].data);
				saved_screens[i].data = NULL;
			}
			retval = 0;
			break;
		case FB_SCREEN_YIELD:
		case FB_SCREEN_RECLAIM:
			retval = EOK;
			break;
		default:
			retval = EINVAL;
		}
		ipc_answer_0(callid, retval);
	}
}

int ega_init(void)
{
	void *ega_ph_addr;
	size_t sz;
	
	sysarg_t paddr;
	if (sysinfo_get_value("fb.address.physical", &paddr) != EOK)
		return -1;
	
	sysarg_t width;
	if (sysinfo_get_value("fb.width", &width) != EOK)
		return -1;
	
	sysarg_t height;
	if (sysinfo_get_value("fb.width", &height) != EOK)
		return -1;
	
	sysarg_t blinking;
	if (sysinfo_get_value("fb.blinking", &blinking) != EOK)
		blinking = false;
	
	ega_ph_addr = (void *) paddr;
	scr_width = width;
	scr_height = height;
	if (blinking) {
		ega_normal_color &= 0x77;
		ega_inverted_color &= 0x77;
	}
	
	style = NORMAL_COLOR;
	
	iospace_enable(task_get_id(), (void *) EGA_IO_BASE, 2);
	
	sz = scr_width * scr_height * 2;
	scr_addr = as_get_mappable_page(sz);
	
	if (physmem_map(ega_ph_addr, scr_addr, ALIGN_UP(sz, PAGE_SIZE) >>
	    PAGE_WIDTH, AS_AREA_READ | AS_AREA_WRITE) != 0)
		return -1;

	async_set_client_connection(ega_client_connection);

	return 0;
}


/**
 * @}
 */
