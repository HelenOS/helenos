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

/** @addtogroup console
 * @{ 
 */
/** @file
 */

#include <ipc/fb.h>
#include <ipc/ipc.h>
#include <async.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <align.h>

#include "console.h"
#include "gcons.h"

#define CONSOLE_TOP      66
#define CONSOLE_MARGIN   6

#define STATUS_START    110
#define STATUS_TOP      8
#define STATUS_SPACE    4
#define STATUS_WIDTH    48
#define STATUS_HEIGHT   48

#define MAIN_COLOR      0xffffff

static int use_gcons = 0;
static ipcarg_t xres,yres;

enum butstate {
	CONS_DISCONNECTED = 0,
	CONS_SELECTED,
	CONS_IDLE,
	CONS_HAS_DATA,
	CONS_KERNEL,
	CONS_DISCONNECTED_SEL,
	CONS_LAST
};

static int console_vp;
static int cstatus_vp[CONSOLE_COUNT];
static enum butstate console_state[CONSOLE_COUNT];

static int fbphone;

/** List of pixmaps identifying these icons */
static int ic_pixmaps[CONS_LAST] = {-1,-1,-1,-1,-1,-1};
static int animation = -1;

static int active_console = 0;

static void vp_switch(int vp)
{
	async_msg(fbphone,FB_VIEWPORT_SWITCH, vp);
}

/** Create view port */
static int vp_create(unsigned int x, unsigned int y, 
		     unsigned int width, unsigned int height)
{
	return async_req_2(fbphone, FB_VIEWPORT_CREATE,
			   (x << 16) | y, (width << 16) | height,
			   NULL, NULL);
}

static void clear(void)
{
	async_msg(fbphone, FB_CLEAR, 0);
	
}

static void set_style(int fgcolor, int bgcolor)
{
	async_msg_2(fbphone, FB_SET_STYLE, fgcolor, bgcolor);
}

/** Transparent putchar */
static void tran_putch(char c, int row, int col)
{
	async_msg_3(fbphone, FB_TRANS_PUTCHAR, c, row, col);
}

/** Redraw the button showing state of a given console */
static void redraw_state(int consnum)
{
	char data[5];
	int i;
	enum butstate state = console_state[consnum];

	vp_switch(cstatus_vp[consnum]);
	if (ic_pixmaps[state] != -1)
		async_msg_2(fbphone, FB_VP_DRAW_PIXMAP, cstatus_vp[consnum], ic_pixmaps[state]);

 	if (state != CONS_DISCONNECTED && state != CONS_KERNEL && state != CONS_DISCONNECTED_SEL) {
 		snprintf(data, 5, "%d", consnum+1);
 		for (i=0;data[i];i++)
 			tran_putch(data[i], 1, 2+i);
 	}
}

/** Notification run on changing console (except kernel console) */
void gcons_change_console(int consnum)
{
	int i;

	if (!use_gcons)
		return;

	if (active_console == KERNEL_CONSOLE) {
		for (i=0; i < CONSOLE_COUNT; i++)
			redraw_state(i);
		if (animation != -1)
			async_msg(fbphone, FB_ANIM_START, animation);
	} else {
		if (console_state[active_console] == CONS_DISCONNECTED_SEL)
			console_state[active_console] = CONS_DISCONNECTED;
		else
			console_state[active_console] = CONS_IDLE;
		redraw_state(active_console);
	}
	active_console = consnum;

	if (console_state[consnum] == CONS_DISCONNECTED) {
		console_state[consnum] = CONS_DISCONNECTED_SEL;
		redraw_state(consnum);
	} else
		console_state[consnum] = CONS_SELECTED;
	redraw_state(consnum);

	vp_switch(console_vp);
}

/** Notification function that gets called on new output to virtual console */
void gcons_notify_char(int consnum)
{
	if (!use_gcons)
		return;

	if (consnum == active_console || console_state[consnum] == CONS_HAS_DATA)
		return;

	console_state[consnum] = CONS_HAS_DATA;

	if (active_console == KERNEL_CONSOLE)
		return;

	redraw_state(consnum);
	
	vp_switch(console_vp);
}

/** Notification function called on service disconnect from console */
void gcons_notify_disconnect(int consnum)
{
	if (!use_gcons)
		return;
	if (active_console == consnum)
		console_state[consnum] = CONS_DISCONNECTED_SEL;
	else
		console_state[consnum] = CONS_DISCONNECTED;

	if (active_console == KERNEL_CONSOLE)
		return;

	redraw_state(consnum);
	vp_switch(console_vp);
}

/** Notification function called on console connect */
void gcons_notify_connect(int consnum)
{
	if (!use_gcons)
		return;
	if (active_console == consnum)
		console_state[consnum] = CONS_SELECTED;
	else
		console_state[consnum] = CONS_IDLE;

	if (active_console == KERNEL_CONSOLE)
		return;

	redraw_state(consnum);
	vp_switch(console_vp);
}

/** Change to kernel console */
void gcons_in_kernel(void)
{
	if (console_state[active_console] == CONS_DISCONNECTED_SEL)
		console_state[active_console] = CONS_DISCONNECTED;
	else
		console_state[active_console] = CONS_IDLE;
	redraw_state(active_console);

	if (animation != -1)
		async_msg(fbphone, FB_ANIM_STOP, animation);

	active_console = KERNEL_CONSOLE; /* Set to kernel console */
	vp_switch(0);
}


static inline int limit(int a,int left, int right)
{
	if (a < left)
		a = left;
	if (a >= right)
		a = right - 1;
	return a;
}

int mouse_x, mouse_y;
int btn_pressed, btn_x, btn_y;

/** Handle mouse move
 *
 * @param dx Delta X of mouse move
 * @param dy Delta Y of mouse move
 */
void gcons_mouse_move(int dx, int dy)
{
	mouse_x = limit(mouse_x+dx, 0, xres);
	mouse_y = limit(mouse_y+dy, 0, yres);

	async_msg_2(fbphone, FB_POINTER_MOVE, mouse_x, mouse_y);
}

static int gcons_find_conbut(int x, int y)
{
	int status_start = STATUS_START + (xres-800) / 2;;

	if (y < STATUS_TOP || y >= STATUS_TOP+STATUS_HEIGHT)
		return -1;
	
	if (x < status_start)
		return -1;
	
	if (x >= status_start + (STATUS_WIDTH+STATUS_SPACE)*CONSOLE_COUNT)
		return -1;
	if (((x - status_start) % (STATUS_WIDTH+STATUS_SPACE)) < STATUS_SPACE)
		return -1;
	
	return (x-status_start) / (STATUS_WIDTH+STATUS_SPACE);
}

/** Handle mouse click
 *
 * @param state New state (1-pressed, 0-depressed)
 */
int gcons_mouse_btn(int state)
{
	int conbut;

	if (state) {
		conbut = gcons_find_conbut(mouse_x, mouse_y);
		if (conbut != -1) {
			btn_pressed = 1;
			btn_x = mouse_x;
			btn_y = mouse_y;
		}
		return -1;
	} 
	if (!state && !btn_pressed)
		return -1;
	btn_pressed = 0;

	conbut = gcons_find_conbut(mouse_x, mouse_y);
	if (conbut == gcons_find_conbut(btn_x, btn_y))
		return conbut;
	return -1;
}


/** Draw a PPM pixmap to framebuffer
 *
 * @param logo Pointer to PPM data
 * @param size Size of PPM data
 * @param x Coordinate of upper left corner
 * @param y Coordinate of upper left corner
 */
static void draw_pixmap(char *logo, size_t size, int x, int y)
{
	char *shm;
	int rc;

	/* Create area */
	shm = mmap(NULL, size, PROTO_READ | PROTO_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (shm == MAP_FAILED)
		return;

	memcpy(shm, logo, size);
	/* Send area */
	rc = async_req_2(fbphone, FB_PREPARE_SHM, (ipcarg_t)shm, 0, NULL, NULL);
	if (rc)
		goto exit;
	rc = async_req_3(fbphone, IPC_M_AS_AREA_SEND, (ipcarg_t)shm, 0, PROTO_READ, NULL, NULL, NULL);
	if (rc)
		goto drop;
	/* Draw logo */
	async_msg_2(fbphone, FB_DRAW_PPM, x, y);
drop:
	/* Drop area */
	async_msg(fbphone, FB_DROP_SHM, 0);
exit:       
	/* Remove area */
	munmap(shm, size);
}

extern char _binary_helenos_ppm_start[0];
extern int _binary_helenos_ppm_size;
extern char _binary_nameic_ppm_start[0];
extern int _binary_nameic_ppm_size;
/** Redraws console graphics  */
static void gcons_redraw_console(void)
{
	int i;

	if (!use_gcons)
		return;
	
	vp_switch(0);
	set_style(MAIN_COLOR, MAIN_COLOR);
	clear();
	draw_pixmap(_binary_helenos_ppm_start, (size_t)&_binary_helenos_ppm_size, xres-66, 2);
	draw_pixmap(_binary_nameic_ppm_start, (size_t)&_binary_nameic_ppm_size, 5, 17);

	for (i=0;i < CONSOLE_COUNT; i++) 
		redraw_state(i);
	vp_switch(console_vp);
}

/** Creates a pixmap on framebuffer
 *
 * @param data PPM data
 * @param size PPM data size
 * @return Pixmap identification
 */
static int make_pixmap(char *data, int size)
{
	char *shm;
	int rc;
	int pxid = -1;

	/* Create area */
	shm = mmap(NULL, size, PROTO_READ | PROTO_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (shm == MAP_FAILED)
		return -1;

	memcpy(shm, data, size);
	/* Send area */
	rc = async_req_2(fbphone, FB_PREPARE_SHM, (ipcarg_t)shm, 0, NULL, NULL);
	if (rc)
		goto exit;
	rc = async_req_3(fbphone, IPC_M_AS_AREA_SEND, (ipcarg_t)shm, 0, PROTO_READ, NULL, NULL, NULL);
	if (rc)
		goto drop;

	/* Obtain pixmap */
	rc = async_req(fbphone, FB_SHM2PIXMAP, 0, NULL);
	if (rc < 0)
		goto drop;
	pxid = rc;
drop:
	/* Drop area */
	async_msg(fbphone, FB_DROP_SHM, 0);
exit:       
	/* Remove area */
	munmap(shm, size);

	return pxid;
}

extern char _binary_anim_1_ppm_start[0];
extern int _binary_anim_1_ppm_size;
extern char _binary_anim_2_ppm_start[0];
extern int _binary_anim_2_ppm_size;
extern char _binary_anim_3_ppm_start[0];
extern int _binary_anim_3_ppm_size;
extern char _binary_anim_4_ppm_start[0];
extern int _binary_anim_4_ppm_size;
static void make_anim(void)
{
	int an;
	int pm;

	an = async_req(fbphone, FB_ANIM_CREATE, cstatus_vp[KERNEL_CONSOLE], NULL);
	if (an < 0)
		return;

	pm = make_pixmap(_binary_anim_1_ppm_start, (int)&_binary_anim_1_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);

	pm = make_pixmap(_binary_anim_2_ppm_start, (int)&_binary_anim_2_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);

	pm = make_pixmap(_binary_anim_3_ppm_start, (int)&_binary_anim_3_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);

	pm = make_pixmap(_binary_anim_4_ppm_start, (int)&_binary_anim_4_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);

	async_msg(fbphone, FB_ANIM_START, an);

	animation = an;
}

extern char _binary_cons_selected_ppm_start[0];
extern int _binary_cons_selected_ppm_size;
extern char _binary_cons_idle_ppm_start[0];
extern int _binary_cons_idle_ppm_size;
extern char _binary_cons_has_data_ppm_start[0];
extern int _binary_cons_has_data_ppm_size;
extern char _binary_cons_kernel_ppm_start[0];
extern int _binary_cons_kernel_ppm_size;
/** Initialize nice graphical console environment */
void gcons_init(int phone)
{
	int rc;
	int i;
	int status_start = STATUS_START;

	fbphone = phone;

	rc = async_req_2(phone, FB_GET_RESOLUTION, 0, 0, &xres, &yres);
	if (rc)
		return;
	
	if (xres < 800 || yres < 600)
		return;

	/* create console viewport */
	/* Align width & height to character size */
	console_vp = vp_create(CONSOLE_MARGIN, CONSOLE_TOP, 
			       ALIGN_DOWN(xres-2*CONSOLE_MARGIN, 8),
			       ALIGN_DOWN(yres-(CONSOLE_TOP+CONSOLE_MARGIN),16));
	if (console_vp < 0)
		return;
	
	/* Create status buttons */
	status_start += (xres-800) / 2;
	for (i=0; i < CONSOLE_COUNT; i++) {
		cstatus_vp[i] = vp_create(status_start+CONSOLE_MARGIN+i*(STATUS_WIDTH+STATUS_SPACE),
					  STATUS_TOP, STATUS_WIDTH, STATUS_HEIGHT);
		if (cstatus_vp[i] < 0)
			return;
		vp_switch(cstatus_vp[i]);
		set_style(0x202020, 0xffffff);
	}
	
	/* Initialize icons */
	ic_pixmaps[CONS_SELECTED] = make_pixmap(_binary_cons_selected_ppm_start,
					      (int)&_binary_cons_selected_ppm_size);
	ic_pixmaps[CONS_IDLE] = make_pixmap(_binary_cons_idle_ppm_start,
					      (int)&_binary_cons_idle_ppm_size);
	ic_pixmaps[CONS_HAS_DATA] = make_pixmap(_binary_cons_has_data_ppm_start,
						(int)&_binary_cons_has_data_ppm_size);
	ic_pixmaps[CONS_DISCONNECTED] = make_pixmap(_binary_cons_idle_ppm_start,
					      (int)&_binary_cons_idle_ppm_size);
	ic_pixmaps[CONS_KERNEL] = make_pixmap(_binary_cons_kernel_ppm_start,
					      (int)&_binary_cons_kernel_ppm_size);
	ic_pixmaps[CONS_DISCONNECTED_SEL] = ic_pixmaps[CONS_SELECTED];
	
	make_anim();

	use_gcons = 1;
	console_state[0] = CONS_DISCONNECTED_SEL;
	console_state[KERNEL_CONSOLE] = CONS_KERNEL;
	gcons_redraw_console();
}
 
/** @}
 */

