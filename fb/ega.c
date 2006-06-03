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

#include "ega.h"
#include "../console/screenbuffer.h"
#include "main.h"

/* Allow only 1 connection */
static int client_connected = 0;

static unsigned int scr_width;
static unsigned int scr_height;
static char *scr_addr;

static unsigned int style = 0x0f;

static void clrscr(void)
{
	int i;
	
	for (i=0; i < scr_width*scr_height; i++) {
		scr_addr[i*2] = ' ';
		scr_addr[i*2+1] = style;
	}
}

static void printchar(char c, unsigned int row, unsigned int col)
{
	scr_addr[(row*scr_width + col)*2] = c;
	scr_addr[(row*scr_width + col)*2+1] = style;
}

static void draw_text_data(keyfield_t *data)
{
	int i;

	for (i=0; i < scr_width*scr_height; i++) {
		scr_addr[i*2] = data[i].character;
		if (data[i].style.fg_color > data[i].style.bg_color)
			scr_addr[i*2+1] = 0x0f;
		else
			scr_addr[i*2+1] = 0xf0;
	}
}

static void ega_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	int retval;
	ipc_callid_t callid;
	ipc_call_t call;
	char c;
	unsigned int row, col;
	int bgcolor,fgcolor;
	keyfield_t *interbuf = NULL;
	size_t intersize = 0;

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
		case IPC_M_AS_AREA_SEND:
			/* We accept one area for data interchange */
			intersize = IPC_GET_ARG2(call);
			if (intersize >= scr_width*scr_height*sizeof(*interbuf)) {
				receive_comm_area(callid,&call,(void **)&interbuf);
				continue;
			}
			retval = EINVAL;
			break;
		case FB_DRAW_TEXT_DATA:
			if (!interbuf) {
				retval = EINVAL;
				break;
			}
			draw_text_data(interbuf);
			retval = 0;
			break;
		case FB_GET_CSIZE:
			ipc_answer_fast(callid, 0, scr_height, scr_width);
			continue;
		case FB_CLEAR:
			clrscr();
			retval = 0;
			break;
		case FB_PUTCHAR:
			c = IPC_GET_ARG1(call);
			row = IPC_GET_ARG2(call);
			col = IPC_GET_ARG3(call);
			if (col >= scr_width || row >= scr_height) {
				retval = EINVAL;
				break;
			}
			printchar(c,row,col);
			retval = 0;
			break;
		case FB_SET_STYLE:
			fgcolor = IPC_GET_ARG1(call);
			bgcolor = IPC_GET_ARG2(call);
			if (fgcolor > bgcolor)
				style = 0x0f;
			else
				style = 0xf0;
		default:
			retval = ENOENT;
		}
		ipc_answer_fast(callid,retval,0,0);
	}
}

int ega_init(void)
{
	void *ega_ph_addr;
	size_t sz;


	ega_ph_addr=(void *)sysinfo_value("fb.address.physical");
	scr_width=sysinfo_value("fb.width");
	scr_height=sysinfo_value("fb.height");

	sz = scr_width*scr_height*2;
	scr_addr = as_get_mappable_page(sz);

	map_physmem(ega_ph_addr, scr_addr, ALIGN_UP(sz,PAGE_SIZE)>>PAGE_WIDTH,
		    AS_AREA_READ | AS_AREA_WRITE);

	async_set_client_connection(ega_client_connection);

	clrscr();

	return 0;
}
