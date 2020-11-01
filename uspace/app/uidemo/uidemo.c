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

#include <gfx/color.h>
#include <gfx/coord.h>
#include <gfx/render.h>
#include <io/pos_event.h>
#include <stdio.h>
#include <str.h>
#include <task.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include "uidemo.h"

static void wnd_close(ui_window_t *, void *);
static void wnd_pos(ui_window_t *, void *, pos_event_t *pos);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.pos = wnd_pos
};

static void pb_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t pbutton_cb = {
	.clicked = pb_clicked
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (demo)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	ui_quit(demo->ui);
}

/** Window position event.
 *
 * @param window Window
 * @param arg Argument (demo)
 */
static void wnd_pos(ui_window_t *window, void *arg, pos_event_t *event)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	/* Make sure we don't process events until fully initialized */
	if (demo->fixed == NULL)
		return;

	ui_fixed_pos_event(demo->fixed, event);
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
		rc = ui_label_set_text(demo->label, "Confirmed");
		if (rc != EOK)
			printf("Error changing label text.\n");
		(void) ui_label_paint(demo->label);
	} else {
		rc = ui_label_set_text(demo->label, "Cancelled");
		if (rc != EOK)
			printf("Error changing label text.\n");
		(void) ui_label_paint(demo->label);
	}
}

/** Run UI demo on display server. */
static errno_t ui_demo(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_demo_t demo;
	gfx_rect_t rect;
	gfx_rect_t app_rect;
	gfx_color_t *color;
	ui_resource_t *ui_res;
	gfx_context_t *gc;
	errno_t rc;

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "UI Demo";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 220;
	params.rect.p1.y = 100;

	memset((void *) &demo, 0, sizeof(demo));
	demo.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &demo);
	demo.window = window;

	task_retval(0);

	ui_res = ui_window_get_res(window);
	gc = ui_window_get_gc(window);
	ui_window_get_app_rect(window, &app_rect);

	rc = ui_fixed_create(&demo.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

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

	rc = ui_fixed_add(demo.fixed, ui_label_ctl(demo.label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

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

	rc = ui_fixed_add(demo.fixed, ui_pbutton_ctl(demo.pb1));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

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

	rc = ui_fixed_add(demo.fixed, ui_pbutton_ctl(demo.pb2));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

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

	rc = gfx_fill_rect(gc, &app_rect);
	if (rc != EOK) {
		printf("Error filling background.\n");
		return rc;
	}

	rc = ui_fixed_paint(demo.fixed);
	if (rc != EOK) {
		printf("Error painting UI controls.\n");
		return rc;
	}

	ui_run(ui);

	ui_fixed_destroy(demo.fixed);
	ui_window_destroy(window);
	ui_destroy(ui);

	return EOK;
}

static void print_syntax(void)
{
	printf("Syntax: uidemo [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_DISPLAY_DEFAULT;
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

			display_spec = argv[i++];
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

	rc = ui_demo(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
