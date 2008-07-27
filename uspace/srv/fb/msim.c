/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Martin Decky
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

/** @defgroup msimfb MSIM text console
 * @brief	HelenOS MSIM text console.
 * @ingroup fbs
 * @{
 */ 
/** @file
 */

#include <async.h>
#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <libc.h>
#include <errno.h>
#include <string.h>
#include <libc.h>
#include <stdio.h>
#include <ipc/fb.h>
#include <sysinfo.h>
#include <as.h>
#include <align.h>
#include <ddi.h>

#include "msim.h"

#define WIDTH 80
#define HEIGHT 25

#define MAX_CONTROL 20

/* Allow only 1 connection */
static int client_connected = 0;

static char *virt_addr;

static void msim_putc(const char c)
{
	*virt_addr = c;
}

static void msim_puts(char *str)
{
	while (*str)
		*virt_addr = *(str++);
}

static void msim_clrscr(void)
{
	msim_puts("\033[2J");
}

static void msim_goto(const unsigned int row, const unsigned int col)
{
	if ((row > HEIGHT) || (col > WIDTH))
		return;
	
	char control[MAX_CONTROL];
	snprintf(control, MAX_CONTROL, "\033[%u;%uf", row + 1, col + 1);
	msim_puts(control);
}

static void msim_set_style(const unsigned int mode)
{
	char control[MAX_CONTROL];
	snprintf(control, MAX_CONTROL, "\033[%um", mode);
	msim_puts(control);
}

static void msim_cursor_disable(void)
{
	msim_puts("\033[?25l");
}

static void msim_cursor_enable(void)
{
	msim_puts("\033[?25h");
}

static void msim_scroll(int i)
{
	if (i > 0) {
		msim_goto(HEIGHT - 1, 0);
		while (i--)
			msim_puts("\033D");
	} else if (i < 0) {
		msim_goto(0, 0);
		while (i++)
			msim_puts("\033M");
	}
}

static void msim_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	int retval;
	ipc_callid_t callid;
	ipc_call_t call;
	char c;
	int lastcol = 0;
	int lastrow = 0;
	int newcol;
	int newrow;
	int fgcolor;
	int bgcolor;
	int i;

	if (client_connected) {
		ipc_answer_0(iid, ELIMIT);
		return;
	}
	
	client_connected = 1;
	ipc_answer_0(iid, EOK);
	
	/* Clear the terminal, set scrolling region
	   to 0 - 25 lines */
	msim_clrscr();
	msim_goto(0, 0);
	msim_puts("\033[0;25r");
	
	while (true) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			client_connected = 0;
			ipc_answer_0(callid, EOK);
			return;
		case FB_PUTCHAR:
			c = IPC_GET_ARG1(call);
			newrow = IPC_GET_ARG2(call);
			newcol = IPC_GET_ARG3(call);
			if ((lastcol != newcol) || (lastrow != newrow))
				msim_goto(newrow, newcol);
			lastcol = newcol + 1;
			lastrow = newrow;
			msim_putc(c);
			retval = 0;
			break;
		case FB_CURSOR_GOTO:
			newrow = IPC_GET_ARG1(call);
			newcol = IPC_GET_ARG2(call);
			msim_goto(newrow, newcol);
			lastrow = newrow;
			lastcol = newcol;
			retval = 0;
			break;
		case FB_GET_CSIZE:
			ipc_answer_2(callid, EOK, HEIGHT, WIDTH);
			continue;
		case FB_CLEAR:
			msim_clrscr();
			retval = 0;
			break;
		case FB_SET_STYLE:
			fgcolor = IPC_GET_ARG1(call);
			bgcolor = IPC_GET_ARG2(call);
			if (fgcolor < bgcolor)
				msim_set_style(0);
			else
				msim_set_style(7);
			retval = 0;
			break;
		case FB_SCROLL:
			i = IPC_GET_ARG1(call);
			if ((i > HEIGHT) || (i < -HEIGHT)) {
				retval = EINVAL;
				break;
			}
			msim_scroll(i);
			msim_goto(lastrow, lastcol);
			retval = 0;
			break;
		case FB_CURSOR_VISIBILITY:
			if(IPC_GET_ARG1(call))
				msim_cursor_enable();
			else
				msim_cursor_disable();
			retval = 0;
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
}

int msim_init(void)
{
	void *phys_addr = (void *) sysinfo_value("fb.address.physical");
	virt_addr = (char *) as_get_mappable_page(1);
	
	physmem_map(phys_addr, virt_addr, 1, AS_AREA_READ | AS_AREA_WRITE);
	
	async_set_client_connection(msim_client_connection);
	return 0;
}

/** 
 * @}
 */
