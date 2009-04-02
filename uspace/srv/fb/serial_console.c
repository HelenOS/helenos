/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
 * Copyright (c) 2008 Pavel Rimsky
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
 * @defgroup serial Serial console
 * @brief    Serial console services (putc, puts, clear screen, cursor goto,...)
 * @{
 */ 

/** @file
 */

#include <stdio.h>
#include <ipc/ipc.h>
#include <async.h>
#include <ipc/fb.h>
#include <bool.h>
#include <errno.h>
#include <console/color.h>
#include <console/style.h>

#include "../console/screenbuffer.h"
#include "main.h"
#include "serial_console.h"

#define MAX_CONTROL 20

static void serial_sgr(const unsigned int mode);

static int scr_width;
static int scr_height;
static bool color = true;	/** True if producing color output. */
static putc_function_t putc_function;

/* Allow only 1 connection */
static int client_connected = 0;

enum sgr_color_index {
	CI_BLACK	= 0,
	CI_RED		= 1,
	CI_GREEN	= 2,
	CI_BROWN	= 3,
	CI_BLUE		= 4,
	CI_MAGENTA	= 5,
	CI_CYAN		= 6,
	CI_WHITE	= 7,
};

enum sgr_command {
	SGR_RESET	= 0,
	SGR_BOLD	= 1,
	SGR_BLINK	= 5,
	SGR_REVERSE	= 7,
	SGR_NORMAL_INT	= 22,
	SGR_BLINK_OFF	= 25,
	SGR_REVERSE_OFF = 27,
	SGR_FGCOLOR	= 30,
	SGR_BGCOLOR	= 40
};

static int color_map[] = {
	[COLOR_BLACK]	= CI_BLACK,
	[COLOR_BLUE]	= CI_RED,
	[COLOR_GREEN]	= CI_GREEN,
	[COLOR_CYAN]	= CI_CYAN,
	[COLOR_RED]	= CI_RED,
	[COLOR_MAGENTA] = CI_MAGENTA,
	[COLOR_YELLOW]	= CI_BROWN,
	[COLOR_WHITE]	= CI_WHITE
};

void serial_puts(char *str)
{
	while (*str)
		putc_function(*(str++));
}

void serial_putchar(wchar_t ch)
{
	(*putc_function)(ch);
}

void serial_goto(const unsigned int row, const unsigned int col)
{
	if ((row > scr_height) || (col > scr_width))
		return;
	
	char control[MAX_CONTROL];
	snprintf(control, MAX_CONTROL, "\033[%u;%uf", row + 1, col + 1);
	serial_puts(control);
}

void serial_clrscr(void)
{
	/* Initialize graphic rendition attributes. */
	serial_sgr(SGR_RESET);
	if (color) {
		serial_sgr(SGR_FGCOLOR + CI_BLACK);
		serial_sgr(SGR_BGCOLOR + CI_WHITE);
	}

	serial_puts("\033[2J");
}

void serial_scroll(int i)
{
	if (i > 0) {
		serial_goto(scr_height - 1, 0);
		while (i--)
			serial_puts("\033D");
	} else if (i < 0) {
		serial_goto(0, 0);
		while (i++)
			serial_puts("\033M");
	}
}

/** ECMA-48 Set Graphics Rendition. */
static void serial_sgr(const unsigned int mode)
{
	char control[MAX_CONTROL];
	snprintf(control, MAX_CONTROL, "\033[%um", mode);
	serial_puts(control);
}

/** Set scrolling region. */
void serial_set_scroll_region(unsigned last_row)
{
	char control[MAX_CONTROL];
	snprintf(control, MAX_CONTROL, "\033[0;%ur", last_row);
	serial_puts(control);
}

void serial_cursor_disable(void)
{
	serial_puts("\033[?25l");
}

void serial_cursor_enable(void)
{
	serial_puts("\033[?25h");
}

void serial_console_init(putc_function_t putc_fn, uint32_t w, uint32_t h)
{
	scr_width = w;
	scr_height = h;
	putc_function = putc_fn;
}

static void serial_set_style(int style)
{
	if (style == STYLE_EMPHASIS) {
		if (color) {
			serial_sgr(SGR_RESET);
			serial_sgr(SGR_FGCOLOR + CI_RED);
			serial_sgr(SGR_BGCOLOR + CI_WHITE);
		}
		serial_sgr(SGR_BOLD);
	} else {
		if (color) {
			serial_sgr(SGR_RESET);
			serial_sgr(SGR_FGCOLOR + CI_BLACK);
			serial_sgr(SGR_BGCOLOR + CI_WHITE);
		}
		serial_sgr(SGR_NORMAL_INT);
	}
}

static void serial_set_idx(unsigned fgcolor, unsigned bgcolor,
    unsigned flags)
{
	if (color) {
		serial_sgr(SGR_RESET);
		serial_sgr(SGR_FGCOLOR + color_map[fgcolor]);
		serial_sgr(SGR_BGCOLOR + color_map[bgcolor]);
	} else {
		if (fgcolor < bgcolor)
			serial_sgr(SGR_RESET);
		else
			serial_sgr(SGR_REVERSE);
	}	
}

static void serial_set_rgb(uint32_t fgcolor, uint32_t bgcolor)
{
	if (fgcolor < bgcolor)
		serial_sgr(SGR_REVERSE_OFF);
	else
		serial_sgr(SGR_REVERSE);	
}

static void serial_set_attrs(const attrs_t *a)
{
	switch (a->t) {
	case at_style: serial_set_style(a->a.s.style); break;
	case at_rgb: serial_set_rgb(a->a.r.fg_color, a->a.r.bg_color); break;
	case at_idx: serial_set_idx(a->a.i.fg_color,
	    a->a.i.bg_color, a->a.i.flags); break;
	default: break;
	}
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
	attrs_t *a0, *a1;

	serial_goto(y, x);
	a0 = &data[0].attrs;
	serial_set_attrs(a0);

	for (j = 0; j < h; j++) {
		if (j > 0 && w != scr_width)
			serial_goto(y, x);

		for (i = 0; i < w; i++) {
			unsigned int col = x + i;
			unsigned int row = y + j;

			field = &data[j * w + i];

			a1 = &field->attrs;
			if (!attrs_same(*a0, *a1))
				serial_set_attrs(a1);
			serial_putchar(field->character);
			a0 = a1;
		}
	}
}

int lastcol = 0;
int lastrow = 0;

/**
 * Main function of the thread serving client connections.
 */
void serial_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	int retval;
	ipc_callid_t callid;
	ipc_call_t call;
	keyfield_t *interbuf = NULL;
	size_t intersize = 0;

	wchar_t c;
	int col, row, w, h;
	int fgcolor;
	int bgcolor;
	int flags;
	int style;
	int i;

	
	if (client_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	
	client_connected = 1;
	ipc_answer_0(iid, EOK);
	
	/* Clear the terminal, set scrolling region
	   to 0 - height rows. */
	serial_clrscr();
	serial_goto(0, 0);
	serial_set_scroll_region(scr_height);
	
	while (true) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			client_connected = 0;
			ipc_answer_0(callid, EOK);
			return;
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
			lastrow = row + h - 1;
			lastcol = col + w;
			retval = 0;
			break;
		case FB_PUTCHAR:
			c = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			col = IPC_GET_ARG3(call);
			if ((lastcol != col) || (lastrow != row))
				serial_goto(row, col);
			lastcol = col + 1;
			lastrow = row;
			serial_putchar(c);
			retval = 0;
			break;
		case FB_CURSOR_GOTO:
			row = IPC_GET_ARG1(call);
			col = IPC_GET_ARG2(call);
			serial_goto(row, col);
			lastrow = row;
			lastcol = col;
			retval = 0;
			break;
		case FB_GET_CSIZE:
			ipc_answer_2(callid, EOK, scr_height, scr_width);
			continue;
		case FB_CLEAR:
			serial_clrscr();
			retval = 0;
			break;
		case FB_SET_STYLE:
			style = IPC_GET_ARG1(call);
			serial_set_style(style);
			retval = 0;
			break;
		case FB_SET_COLOR:
			fgcolor = IPC_GET_ARG1(call);
			bgcolor = IPC_GET_ARG2(call);
			flags = IPC_GET_ARG3(call);

			serial_set_idx(fgcolor, bgcolor, flags);
			retval = 0;
			break;
		case FB_SET_RGB_COLOR:
			fgcolor = IPC_GET_ARG1(call);
			bgcolor = IPC_GET_ARG2(call);

			serial_set_rgb(fgcolor, bgcolor);
			retval = 0;
			break;
		case FB_SCROLL:
			i = IPC_GET_ARG1(call);
			if ((i > scr_height) || (i < -scr_height)) {
				retval = EINVAL;
				break;
			}
			serial_scroll(i);
			serial_goto(lastrow, lastcol);
			retval = 0;
			break;
		case FB_CURSOR_VISIBILITY:
			if(IPC_GET_ARG1(call))
				serial_cursor_enable();
			else
				serial_cursor_disable();
			retval = 0;
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
}

/** 
 * @}
 */
