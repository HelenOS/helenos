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

#include <async.h>
#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <libc.h>
#include <errno.h>
#include <string.h>
#include <libc.h>
#include <stdio.h>

#include "sysio.h"

#define WIDTH 80
#define HEIGHT 25

/* Allow only 1 connection */
static int client_connected = 0;

static void sysput(char c)
{
	__SYSCALL3(SYS_IO, 1, (sysarg_t)&c, (sysarg_t) 1);
}

static void sysputs(char *s)
{
	while (*s) {
		sysput(*(s++));
	}
//	__SYSCALL3(SYS_IO, 1, (sysarg_t)s, strlen(s));
}

/** Send clearscreen sequence to console */
static void clrscr(void)
{
	sysputs("\033[2J");
}

/** Send ansi sequence to console to change cursor position */
static void curs_goto(unsigned int row, unsigned int col)
{
	char control[20];

	if (row > 200 || col > 200)
		return;

	snprintf(control, 20, "\033[%d;%df",row+1, col+1);
	sysputs(control);
}

static void set_style(int mode)
{
	char control[20];
	
	snprintf(control, 20, "\033[%dm", mode);
	sysputs(control);
}

static void scroll(int i)
{
	if (i > 0) {
		curs_goto(HEIGHT-1, 0);
		while (i--)
			sysputs("\033D");
	} else if (i < 0) {
		curs_goto(0,0);
		while (i++)
			sysputs("\033M");
	}
}

/** ANSI terminal emulation main thread */
static void sysio_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	int retval;
	ipc_callid_t callid;
	ipc_call_t call;
	char c;
	int lastcol=0;
	int lastrow=0;
	int newcol,newrow;
	int fgcolor,bgcolor;
	int i;

	if (client_connected) {
		ipc_answer_fast(iid, ELIMIT, 0,0);
		return;
	}
	client_connected = 1;
	ipc_answer_fast(iid, 0, 0, 0); /* Accept connection */
	while (1) {
		callid = async_get_call(&call);
 		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			client_connected = 0;
			ipc_answer_fast(callid,0,0,0);
			return; /* Exit thread */
		case FB_PUTCHAR:
			c = IPC_GET_ARG1(call);
			newrow = IPC_GET_ARG2(call);
			newcol = IPC_GET_ARG3(call);
			if (lastcol != newcol || lastrow!=newrow) 
				curs_goto(newrow, newcol);
			lastcol = newcol + 1;
			lastrow = newrow;
			sysput(c);
			retval = 0;
			break;
 		case FB_CURSOR_GOTO:
			newrow = IPC_GET_ARG1(call);
			newcol = IPC_GET_ARG2(call);
			curs_goto(newrow, newcol);
			lastrow = newrow;
			lastcol = newcol;
			break;
		case FB_GET_CSIZE:
			ipc_answer_fast(callid, 0, HEIGHT, WIDTH);
			continue;
		case FB_CLEAR:
			clrscr();
			retval = 0;
			break;
		case FB_SET_STYLE:
			fgcolor = IPC_GET_ARG1(call);
			bgcolor = IPC_GET_ARG2(call);
			if (fgcolor < bgcolor)
				set_style(0);
			else
				set_style(7);
			retval = 0;
			break;
		case FB_SCROLL:
			i = IPC_GET_ARG1(call);
			if (i > HEIGHT || i < -HEIGHT) {
				retval = EINVAL;
				break;
			}
			scroll(i);
			curs_goto(lastrow, lastcol);
			retval = 0;
			break;

		default:
			retval = ENOENT;
		}
		ipc_answer_fast(callid,retval,0,0);
	}
}

/** ANSI terminal emulation initialization */
void sysio_init(void)
{
	async_set_client_connection(sysio_client_connection);
	clrscr();
	curs_goto(0,0);
	/* Set scrolling region to 0-25 lines */
	sysputs("\033[0;25r");
}
