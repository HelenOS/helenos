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

#include <ipc/fb.h>
#include <ipc/ipc.h>

#include "console.h"

#define CONSOLE_TOP      50
#define CONSOLE_MARGIN   10

#define STATUS_SPACE    20
#define STATUS_WIDTH    40
#define STATUS_HEIGHT   30

static int use_gcons = 0;
static ipcarg_t xres,yres;

static int console_vp;
static int cstatus_vp[CONSOLE_COUNT];

static int fbphone;

static void vp_switch(int vp)
{
	ipc_call_sync_2(fbphone,FB_VIEWPORT_SWITCH, vp, 0, NULL, NULL);
}

static int vp_create(unsigned int x, unsigned int y, 
		     unsigned int width, unsgined int height)
{
	return  ipc_call_sync_2(fbphone, (x << 16) | y, (width << 16) | height,
				NULL, NULL);
}

static void fb_clear(void)
{
	ipc_call_sync_2(fbphone, FB_CLEAR, 0, 0, NULL, NULL);
	
}

static void fb_set_style(int fgcolor, int bgcolor)
{
	ipc_call_sync_2(fbphone, );
}

void gcons_change_console(int consnum)
{
	if (!use_gcons)
		return;

	vp_switch(console_vp);
}

void gcons_notify_char(int consnum)
{
	if (!use_gcons)
		return;

	vp_switch(console_vp);
}

void gcons_redraw_console(int phone)
{
	if (!use_gcons)
		return;
	
	vp_switch(0);
	/* Set style...*/
	fb_clear();
	
	vp_switch(console_vp);
}

/** Initialize nice graphical console environment */
void gcons_init(int phone)
{
	int rc;
	int i;

	fbphone = phone;

	rc = ipc_call_sync_2(phone, FB_GET_RESOLUTION, 0, 0, &xres, &yres);
	if (rc)
		return;
	
	if (xres < 800 || yres < 600)
		return;

	/* create console viewport */
	console_vp = vp_create(CONSOLE_MARGIN, CONSOLE_TOP, xres-2*CONSOLE_MARGIN,
			       yres-(CONSOLE_TOP+CONSOLE_MARGIN));
	if (console_vp < 0)
		return;
	
	/* Create status buttons */
	for (i=0; i < CONSOLE_COUNT; i++) {
		cstatus_vp[i] = vp_create(phone, CONSOLE_MARGIN+i*(STATUS_WIDTH+STATUS_SPACE),
					  CONSOLE_MARGIN, STATUS_WIDTH, STATUS_HEIGHT);
		if (cstatus_vp[i] < 0)
			return;
	}
	
	use_gcons = 1;
	gcons_draw_console();
}
