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
#include <bool.h>

#include "console.h"
#include "gcons.h"

#define CONSOLE_TOP     66
#define CONSOLE_MARGIN  6

#define STATUS_START   110
#define STATUS_TOP     8
#define STATUS_SPACE   4
#define STATUS_WIDTH   48
#define STATUS_HEIGHT  48

#define MAIN_COLOR  0xffffff

static bool use_gcons = false;
static ipcarg_t xres;
static ipcarg_t yres;

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
static int ic_pixmaps[CONS_LAST] = {-1, -1, -1, -1, -1, -1};
static int animation = -1;

static size_t active_console = 0;

size_t mouse_x;
size_t mouse_y;

bool btn_pressed;
size_t btn_x;
size_t btn_y;

static void vp_switch(int vp)
{
	async_msg_1(fbphone, FB_VIEWPORT_SWITCH, vp);
}

/** Create view port */
static int vp_create(size_t x, size_t y, size_t width, size_t height)
{
	return async_req_2_0(fbphone, FB_VIEWPORT_CREATE, (x << 16) | y,
	    (width << 16) | height);
}

static void clear(void)
{
	async_msg_0(fbphone, FB_CLEAR);
}

static void set_rgb_color(uint32_t fgcolor, uint32_t bgcolor)
{
	async_msg_2(fbphone, FB_SET_RGB_COLOR, fgcolor, bgcolor);
}

/** Transparent putchar */
static void tran_putch(wchar_t ch, size_t col, size_t row)
{
	async_msg_3(fbphone, FB_PUTCHAR, ch, col, row);
}

/** Redraw the button showing state of a given console */
static void redraw_state(size_t index)
{
	vp_switch(cstatus_vp[index]);
	
	enum butstate state = console_state[index];
	
	if (ic_pixmaps[state] != -1)
		async_msg_2(fbphone, FB_VP_DRAW_PIXMAP, cstatus_vp[index],
		    ic_pixmaps[state]);
	
	if ((state != CONS_DISCONNECTED) && (state != CONS_KERNEL)
	    && (state != CONS_DISCONNECTED_SEL)) {
		
		char data[5];
		snprintf(data, 5, "%u", index + 1);
		
		size_t i;
		for (i = 0; data[i] != 0; i++)
			tran_putch(data[i], 2 + i, 1);
	}
}

/** Notification run on changing console (except kernel console) */
void gcons_change_console(size_t index)
{
	if (!use_gcons)
		return;
	
	if (active_console == KERNEL_CONSOLE) {
		size_t i;
		
		for (i = 0; i < CONSOLE_COUNT; i++)
			redraw_state(i);
		
		if (animation != -1)
			async_msg_1(fbphone, FB_ANIM_START, animation);
	} else {
		if (console_state[active_console] == CONS_DISCONNECTED_SEL)
			console_state[active_console] = CONS_DISCONNECTED;
		else
			console_state[active_console] = CONS_IDLE;
		
		redraw_state(active_console);
	}
	
	active_console = index;
	
	if ((console_state[index] == CONS_DISCONNECTED)
	    || (console_state[index] == CONS_DISCONNECTED_SEL))
		console_state[index] = CONS_DISCONNECTED_SEL;
	else
		console_state[index] = CONS_SELECTED;
	
	redraw_state(index);
	vp_switch(console_vp);
}

/** Notification function that gets called on new output to virtual console */
void gcons_notify_char(size_t index)
{
	if (!use_gcons)
		return;
	
	if ((index == active_console)
	    || (console_state[index] == CONS_HAS_DATA))
		return;
	
	console_state[index] = CONS_HAS_DATA;
	
	if (active_console == KERNEL_CONSOLE)
		return;
	
	redraw_state(index);
	vp_switch(console_vp);
}

/** Notification function called on service disconnect from console */
void gcons_notify_disconnect(size_t index)
{
	if (!use_gcons)
		return;
	
	if (index == active_console)
		console_state[index] = CONS_DISCONNECTED_SEL;
	else
		console_state[index] = CONS_DISCONNECTED;
	
	if (active_console == KERNEL_CONSOLE)
		return;
	
	redraw_state(index);
	vp_switch(console_vp);
}

/** Notification function called on console connect */
void gcons_notify_connect(size_t index)
{
	if (!use_gcons)
		return;
	
	if (index == active_console)
		console_state[index] = CONS_SELECTED;
	else
		console_state[index] = CONS_IDLE;
	
	if (active_console == KERNEL_CONSOLE)
		return;
	
	redraw_state(index);
	vp_switch(console_vp);
}

/** Change to kernel console */
void gcons_in_kernel(void)
{
	if (animation != -1)
		async_msg_1(fbphone, FB_ANIM_STOP, animation);
	
	active_console = KERNEL_CONSOLE;
	vp_switch(0);
}

/** Return x, where left <= x <= right && |a-x| == min(|a-x|) is smallest */
static inline int limit(size_t a, size_t left, size_t right)
{
	if (a < left)
		a = left;
	
	if (a >= right)
		a = right - 1;
	
	return a;
}

/** Handle mouse move
 *
 * @param dx Delta X of mouse move
 * @param dy Delta Y of mouse move
 */
void gcons_mouse_move(ssize_t dx, ssize_t dy)
{
	mouse_x = limit(mouse_x + dx, 0, xres);
	mouse_y = limit(mouse_y + dy, 0, yres);
	
	async_msg_2(fbphone, FB_POINTER_MOVE, mouse_x, mouse_y);
}

static int gcons_find_conbut(int x, int y)
{
	int status_start = STATUS_START + (xres - 800) / 2;
	
	if ((y < STATUS_TOP) || (y >= STATUS_TOP + STATUS_HEIGHT))
		return -1;
	
	if (x < status_start)
		return -1;
	
	if (x >= status_start + (STATUS_WIDTH + STATUS_SPACE) * CONSOLE_COUNT)
		return -1;
	if (((x - status_start) % (STATUS_WIDTH + STATUS_SPACE)) < STATUS_SPACE)
		return -1;
	
	return (x - status_start) / (STATUS_WIDTH + STATUS_SPACE);
}

/** Handle mouse click
 *
 * @param state New state (true - pressed, false - depressed)
 */
int gcons_mouse_btn(bool state)
{
	int conbut;
	
	if (state) {
		conbut = gcons_find_conbut(mouse_x, mouse_y);
		if (conbut != -1) {
			btn_pressed = true;
			btn_x = mouse_x;
			btn_y = mouse_y;
		}
		return -1;
	}
	
	if ((!state) && (!btn_pressed))
		return -1;
	
	btn_pressed = false;
	
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
	shm = mmap(NULL, size, PROTO_READ | PROTO_WRITE, MAP_SHARED |
	    MAP_ANONYMOUS, 0, 0);
	if (shm == MAP_FAILED)
		return;
	
	memcpy(shm, logo, size);
	
	/* Send area */
	rc = async_req_1_0(fbphone, FB_PREPARE_SHM, (ipcarg_t) shm);
	if (rc)
		goto exit;
	
	rc = ipc_share_out_start(fbphone, shm, PROTO_READ);
	if (rc)
		goto drop;
	
	/* Draw logo */
	async_msg_2(fbphone, FB_DRAW_PPM, x, y);
	
drop:
	/* Drop area */
	async_msg_0(fbphone, FB_DROP_SHM);
	
exit:
	/* Remove area */
	munmap(shm, size);
}

extern char _binary_gfx_helenos_ppm_start[0];
extern int _binary_gfx_helenos_ppm_size;
extern char _binary_gfx_nameic_ppm_start[0];
extern int _binary_gfx_nameic_ppm_size;

/** Redraws console graphics */
void gcons_redraw_console(void)
{
	int i;
	
	if (!use_gcons)
		return;
	
	vp_switch(0);
	set_rgb_color(MAIN_COLOR, MAIN_COLOR);
	clear();
	draw_pixmap(_binary_gfx_helenos_ppm_start,
	    (size_t) &_binary_gfx_helenos_ppm_size, xres - 66, 2);
	draw_pixmap(_binary_gfx_nameic_ppm_start,
	    (size_t) &_binary_gfx_nameic_ppm_size, 5, 17);
	
	for (i = 0; i < CONSOLE_COUNT; i++)
		redraw_state(i);
	
	vp_switch(console_vp);
}

/** Creates a pixmap on framebuffer
 *
 * @param data PPM data
 * @param size PPM data size
 *
 * @return Pixmap identification
 *
 */
static int make_pixmap(char *data, size_t size)
{
	char *shm;
	int rc;
	int pxid = -1;
	
	/* Create area */
	shm = mmap(NULL, size, PROTO_READ | PROTO_WRITE, MAP_SHARED |
	    MAP_ANONYMOUS, 0, 0);
	if (shm == MAP_FAILED)
		return -1;
	
	memcpy(shm, data, size);
	
	/* Send area */
	rc = async_req_1_0(fbphone, FB_PREPARE_SHM, (ipcarg_t) shm);
	if (rc)
		goto exit;
	
	rc = ipc_share_out_start(fbphone, shm, PROTO_READ);
	if (rc)
		goto drop;
	
	/* Obtain pixmap */
	rc = async_req_0_0(fbphone, FB_SHM2PIXMAP);
	if (rc < 0)
		goto drop;
	
	pxid = rc;
	
drop:
	/* Drop area */
	async_msg_0(fbphone, FB_DROP_SHM);
	
exit:
	/* Remove area */
	munmap(shm, size);
	
	return pxid;
}

extern char _binary_gfx_anim_1_ppm_start[0];
extern int _binary_gfx_anim_1_ppm_size;
extern char _binary_gfx_anim_2_ppm_start[0];
extern int _binary_gfx_anim_2_ppm_size;
extern char _binary_gfx_anim_3_ppm_start[0];
extern int _binary_gfx_anim_3_ppm_size;
extern char _binary_gfx_anim_4_ppm_start[0];
extern int _binary_gfx_anim_4_ppm_size;

static void make_anim(void)
{
	int an = async_req_1_0(fbphone, FB_ANIM_CREATE, cstatus_vp[KERNEL_CONSOLE]);
	if (an < 0)
		return;
	
	int pm = make_pixmap(_binary_gfx_anim_1_ppm_start,
	    (int) &_binary_gfx_anim_1_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);
	
	pm = make_pixmap(_binary_gfx_anim_2_ppm_start,
	    (int) &_binary_gfx_anim_2_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);
	
	pm = make_pixmap(_binary_gfx_anim_3_ppm_start,
	    (int) &_binary_gfx_anim_3_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);
	
	pm = make_pixmap(_binary_gfx_anim_4_ppm_start,
	    (int) &_binary_gfx_anim_4_ppm_size);
	async_msg_2(fbphone, FB_ANIM_ADDPIXMAP, an, pm);
	
	async_msg_1(fbphone, FB_ANIM_START, an);
	
	animation = an;
}

extern char _binary_gfx_cons_selected_ppm_start[0];
extern int _binary_gfx_cons_selected_ppm_size;
extern char _binary_gfx_cons_idle_ppm_start[0];
extern int _binary_gfx_cons_idle_ppm_size;
extern char _binary_gfx_cons_has_data_ppm_start[0];
extern int _binary_gfx_cons_has_data_ppm_size;
extern char _binary_gfx_cons_kernel_ppm_start[0];
extern int _binary_gfx_cons_kernel_ppm_size;

/** Initialize nice graphical console environment */
void gcons_init(int phone)
{
	fbphone = phone;
	
	int rc = async_req_0_2(phone, FB_GET_RESOLUTION, &xres, &yres);
	if (rc)
		return;
	
	if ((xres < 800) || (yres < 600))
		return;
	
	/* Create console viewport */
	
	/* Align width & height to character size */
	console_vp = vp_create(CONSOLE_MARGIN, CONSOLE_TOP,
	    ALIGN_DOWN(xres - 2 * CONSOLE_MARGIN, 8),
	    ALIGN_DOWN(yres - (CONSOLE_TOP + CONSOLE_MARGIN), 16));
	
	if (console_vp < 0)
		return;
	
	/* Create status buttons */
	size_t status_start = STATUS_START + (xres - 800) / 2;
	size_t i;
	for (i = 0; i < CONSOLE_COUNT; i++) {
		cstatus_vp[i] = vp_create(status_start + CONSOLE_MARGIN +
		    i * (STATUS_WIDTH + STATUS_SPACE), STATUS_TOP,
		    STATUS_WIDTH, STATUS_HEIGHT);
		
		if (cstatus_vp[i] < 0)
			return;
		
		vp_switch(cstatus_vp[i]);
		set_rgb_color(0x202020, 0xffffff);
	}
	
	/* Initialize icons */
	ic_pixmaps[CONS_SELECTED] =
	    make_pixmap(_binary_gfx_cons_selected_ppm_start,
	    (size_t) &_binary_gfx_cons_selected_ppm_size);
	ic_pixmaps[CONS_IDLE] =
	    make_pixmap(_binary_gfx_cons_idle_ppm_start,
	    (size_t) &_binary_gfx_cons_idle_ppm_size);
	ic_pixmaps[CONS_HAS_DATA] =
	    make_pixmap(_binary_gfx_cons_has_data_ppm_start,
	    (size_t) &_binary_gfx_cons_has_data_ppm_size);
	ic_pixmaps[CONS_DISCONNECTED] =
	    make_pixmap(_binary_gfx_cons_idle_ppm_start,
	    (size_t) &_binary_gfx_cons_idle_ppm_size);
	ic_pixmaps[CONS_KERNEL] =
	    make_pixmap(_binary_gfx_cons_kernel_ppm_start,
	    (size_t) &_binary_gfx_cons_kernel_ppm_size);
	ic_pixmaps[CONS_DISCONNECTED_SEL] = ic_pixmaps[CONS_SELECTED];
	
	make_anim();
	
	use_gcons = true;
	console_state[0] = CONS_DISCONNECTED_SEL;
	console_state[KERNEL_CONSOLE] = CONS_KERNEL;
	
	vp_switch(console_vp);
}

/** @}
 */
