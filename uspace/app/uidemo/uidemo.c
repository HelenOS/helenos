/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup uidemo
 * @{
 */
/** @file User interface demo
 */

#include <display.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include "uidemo.h"

static void wnd_close_event(void *);
static void wnd_kbd_event(void *, kbd_event_t *);
static void wnd_pos_event(void *, pos_event_t *);

static display_wnd_cb_t wnd_cb = {
	.close_event = wnd_close_event,
	.kbd_event = wnd_kbd_event,
	.pos_event = wnd_pos_event
};

static bool quit = false;

/** Print syntax. */
static void print_syntax(void)
{
	printf("Syntax: uidemo [-d <display>]\n");
}

/** Handle window close event. */
static void wnd_close_event(void *arg)
{
	printf("Close event\n");
	quit = true;
}

/** Handle window keyboard event */
static void wnd_kbd_event(void *arg, kbd_event_t *event)
{
	printf("Keyboard event type=%d key=%d\n", event->type, event->key);
	if (event->type == KEY_PRESS)
		quit = true;
}

/** Handle window position event */
static void wnd_pos_event(void *arg, pos_event_t *event)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	gfx_rect_t rect1;
	gfx_rect_t rect2;
	gfx_coord2_t pos;

	rect1.p0.x = 20;
	rect1.p0.y = 50;
	rect1.p1.x = 100;
	rect1.p1.y = 80;

	rect2.p0.x = 120;
	rect2.p0.y = 50;
	rect2.p1.x = 200;
	rect2.p1.y = 80;

	pos.x = event->hpos;
	pos.y = event->vpos;

	if (event->type == POS_PRESS) {
		printf("Button press\n");

		if (gfx_pix_inside_rect(&pos, &rect1)) {
			printf("Press button 1\n");
			ui_pbutton_press(demo->pb1);
			(void) ui_pbutton_paint(demo->pb1);
		}
		if (gfx_pix_inside_rect(&pos, &rect2)) {
			printf("Press button 2\n");
			ui_pbutton_press(demo->pb2);
			(void) ui_pbutton_paint(demo->pb2);
		}
	}

	if (event->type == POS_RELEASE) {
		printf("Button release\n");
		if (gfx_pix_inside_rect(&pos, &rect1)) {
			printf("Release button 1\n");
			ui_pbutton_release(demo->pb1);
			(void) ui_pbutton_paint(demo->pb1);
		}
		if (gfx_pix_inside_rect(&pos, &rect2)) {
			printf("Release button 2\n");
			ui_pbutton_release(demo->pb2);
			(void) ui_pbutton_paint(demo->pb2);
		}
	}
}

/** Run UI demo on display server. */
static errno_t ui_demo_display(const char *display_svc)
{
	display_t *display = NULL;
	gfx_context_t *gc;
	display_wnd_params_t params;
	display_window_t *window = NULL;
	ui_resource_t *ui_res;
	ui_demo_t demo;
	gfx_rect_t rect;
	errno_t rc;

	printf("Init display..\n");

	rc = display_open(display_svc, &display);
	if (rc != EOK) {
		printf("Error opening display.\n");
		return rc;
	}

	display_wnd_params_init(&params);
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 220;
	params.rect.p1.y = 100;

	rc = display_window_create(display, &params, &wnd_cb, (void *) &demo,
	    &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	rc = display_window_get_gc(window, &gc);
	if (rc != EOK) {
		printf("Error getting graphics context.\n");
		return rc;
	}

	task_retval(0);

	rc = ui_resource_create(gc, &ui_res);
	if (rc != EOK) {
		printf("Error creating UI.\n");
		return rc;
	}

	rc = ui_pbutton_create(ui_res, "Confirm", &demo.pb1);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	rect.p0.x = 20;
	rect.p0.y = 50;
	rect.p1.x = 100;
	rect.p1.y = 80;
	ui_pbutton_set_rect(demo.pb1, &rect);

	rc = ui_pbutton_create(ui_res, "Cancel", &demo.pb2);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	rect.p0.x = 120;
	rect.p0.y = 50;
	rect.p1.x = 200;
	rect.p1.y = 80;
	ui_pbutton_set_rect(demo.pb2, &rect);

	rc = ui_pbutton_paint(demo.pb1);
	if (rc != EOK) {
		printf("Error painting button.\n");
		return rc;
	}

	rc = ui_pbutton_paint(demo.pb2);
	if (rc != EOK) {
		printf("Error painting button.\n");
		return rc;
	}

	while (!quit) {
		fibril_usleep(100 * 1000);
	}

	ui_pbutton_destroy(demo.pb1);
	ui_pbutton_destroy(demo.pb2);

	rc = gfx_context_delete(gc);
	if (rc != EOK)
		return rc;

	display_window_destroy(window);
	display_close(display);

	return EOK;
}

int main(int argc, char *argv[])
{
	const char *display_svc = DISPLAY_DEFAULT;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_svc = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i < argc) {
		print_syntax();
		return 1;
	}

	rc = ui_demo_display(display_svc);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
