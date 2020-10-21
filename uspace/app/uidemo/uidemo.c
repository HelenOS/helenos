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
#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/render.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/wdecor.h>
#include "uidemo.h"

static void wnd_close_event(void *);
static void wnd_focus_event(void *);
static void wnd_kbd_event(void *, kbd_event_t *);
static void wnd_pos_event(void *, pos_event_t *);
static void wnd_unfocus_event(void *);

static display_wnd_cb_t wnd_cb = {
	.close_event = wnd_close_event,
	.focus_event = wnd_focus_event,
	.kbd_event = wnd_kbd_event,
	.pos_event = wnd_pos_event,
	.unfocus_event = wnd_unfocus_event
};

static void pb_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t pbutton_cb = {
	.clicked = pb_clicked
};

static void wd_close(ui_wdecor_t *, void *);
static void wd_move(ui_wdecor_t *, void *, gfx_coord2_t *);

static ui_wdecor_cb_t wdecor_cb = {
	.close = wd_close,
	.move = wd_move
};

/** Print syntax. */
static void print_syntax(void)
{
	printf("Syntax: uidemo [-d <display>]\n");
}

/** Handle window close event. */
static void wnd_close_event(void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	printf("Close event\n");
	demo->quit = true;
}

/** Handle window focus event. */
static void wnd_focus_event(void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	if (demo->wdecor != NULL) {
		ui_wdecor_set_active(demo->wdecor, true);
		ui_wdecor_paint(demo->wdecor);
	}
}

/** Handle window keyboard event */
static void wnd_kbd_event(void *arg, kbd_event_t *event)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	printf("Keyboard event type=%d key=%d\n", event->type, event->key);
}

/** Handle window position event */
static void wnd_pos_event(void *arg, pos_event_t *event)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	/* Make sure we don't process events until fully initialized */
	if (demo->wdecor == NULL || demo->pb1 == NULL || demo->pb2 == NULL)
		return;

	ui_wdecor_pos_event(demo->wdecor, event);
	ui_pbutton_pos_event(demo->pb1, event);
	ui_pbutton_pos_event(demo->pb2, event);
}

/** Handle window unfocus event. */
static void wnd_unfocus_event(void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	if (demo->wdecor != NULL) {
		ui_wdecor_set_active(demo->wdecor, false);
		ui_wdecor_paint(demo->wdecor);
	}
}

/** Push button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (demo)
 */
static void pb_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	errno_t rc;

	if (pbutton == demo->pb1) {
		printf("Clicked 'Confirm' button\n");
		rc = ui_label_set_text(demo->label, "Confirmed");
		if (rc != EOK)
			printf("Error changing label text.\n");
		(void) ui_label_paint(demo->label);
	} else {
		printf("Clicked 'Cancel' button\n");
		rc = ui_label_set_text(demo->label, "Cancelled");
		if (rc != EOK)
			printf("Error changing label text.\n");
		(void) ui_label_paint(demo->label);
	}
}

/** Window decoration requested window closure.
 *
 * @param wdecor Window decoration
 * @param arg Argument (demo)
 */
static void wd_close(ui_wdecor_t *wdecor, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	printf("Close window requested\n");
	demo->quit = true;
}

/** Window decoration requested window move.
 *
 * @param wdecor Window decoration
 * @param arg Argument (demo)
 * @param pos Position where the title bar was pressed
 */
static void wd_move(ui_wdecor_t *wdecor, void *arg, gfx_coord2_t *pos)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) display_window_move_req(demo->dwindow, pos);
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
	gfx_color_t *color = NULL;
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

	memset((void *) &demo, 0, sizeof(demo));

	rc = display_window_create(display, &params, &wnd_cb, (void *) &demo,
	    &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	demo.quit = false;
	demo.dwindow = window;

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

	printf("Create window decoration\n");
	rc = ui_wdecor_create(ui_res, "UI Demo", &demo.wdecor);
	if (rc != EOK) {
		printf("Error creating window decoration.\n");
		return rc;
	}

	ui_wdecor_set_rect(demo.wdecor, &params.rect);
	ui_wdecor_set_cb(demo.wdecor, &wdecor_cb, (void *) &demo);

	rc = ui_label_create(ui_res, "Hello there!", &demo.label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	rect.p0.x = 60;
	rect.p0.y = 37;
	rect.p1.x = 160;
	rect.p1.y = 50;
	ui_label_set_rect(demo.label, &rect);
	ui_label_set_halign(demo.label, gfx_halign_center);

	rc = ui_pbutton_create(ui_res, "Confirm", &demo.pb1);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(demo.pb1, &pbutton_cb, (void *) &demo);

	rect.p0.x = 15;
	rect.p0.y = 60;
	rect.p1.x = 105;
	rect.p1.y = 88;
	ui_pbutton_set_rect(demo.pb1, &rect);

	ui_pbutton_set_default(demo.pb1, true);

	rc = ui_pbutton_create(ui_res, "Cancel", &demo.pb2);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(demo.pb2, &pbutton_cb, (void *) &demo);

	rect.p0.x = 115;
	rect.p0.y = 60;
	rect.p1.x = 205;
	rect.p1.y = 88;
	ui_pbutton_set_rect(demo.pb2, &rect);

	rc = gfx_color_new_rgb_i16(0xc8c8, 0xc8c8, 0xc8c8, &color);
	if (rc != EOK) {
		printf("Error allocating color.\n");
		return rc;
	}

	rc = gfx_set_color(gc, color);
	if (rc != EOK) {
		printf("Error setting color.\n");
		return rc;
	}

	rc = gfx_fill_rect(gc, &params.rect);
	if (rc != EOK) {
		printf("Error filling background.\n");
		return rc;
	}

	gfx_color_delete(color);
	color = NULL;

	rc = ui_wdecor_paint(demo.wdecor);
	if (rc != EOK) {
		printf("Error painting window decoration.\n");
		return rc;
	}

	rc = ui_label_paint(demo.label);
	if (rc != EOK) {
		printf("Error painting button.\n");
		return rc;
	}

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

	while (!demo.quit) {
		fibril_usleep(100 * 1000);
	}

	ui_wdecor_destroy(demo.wdecor);
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
