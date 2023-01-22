/*
 * Copyright (c) 2023 Jiri Svoboda
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup launcher
 * @{
 */
/** @file Launcher
 */

#include <errno.h>
#include <gfx/coord.h>
#include <gfximage/tga.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <ui/fixed.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>

#include "images.h"
#include "launcher.h"

#define NAME  "launcher"

static const char *display_spec = UI_DISPLAY_DEFAULT;

static void wnd_close(ui_window_t *, void *);
static void wnd_pos(ui_window_t *, void *, pos_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.pos = wnd_pos
};

static void pb_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t pbutton_cb = {
	.clicked = pb_clicked
};

static int app_launchl(launcher_t *, const char *, ...);

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (launcher)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	launcher_t *launcher = (launcher_t *) arg;

	ui_quit(launcher->ui);
}

/** Window received position event.
 *
 * @param window Window
 * @param arg Argument (launcher)
 * @param event Position event
 */
static void wnd_pos(ui_window_t *window, void *arg, pos_event_t *event)
{
	launcher_t *launcher = (launcher_t *) arg;

	/* Remember ID of device that sent the last event */
	launcher->ev_pos_id = event->pos_id;

	ui_window_def_pos(window, event);
}

/** Push button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (launcher)
 */
static void pb_clicked(ui_pbutton_t *pbutton, void *arg)
{
	launcher_t *launcher = (launcher_t *) arg;

	if (pbutton == launcher->pb1) {
		app_launchl(launcher, "/app/terminal", "-c", "/app/nav", NULL);
	} else if (pbutton == launcher->pb2) {
		app_launchl(launcher, "/app/terminal", "-c", "/app/edit", NULL);
	} else if (pbutton == launcher->pb3) {
		app_launchl(launcher, "/app/terminal", NULL);
	} else if (pbutton == launcher->pb4) {
		app_launchl(launcher, "/app/calculator", NULL);
	} else if (pbutton == launcher->pb5) {
		app_launchl(launcher, "/app/uidemo", NULL);
	} else if (pbutton == launcher->pb6) {
		app_launchl(launcher, "/app/gfxdemo", "ui", NULL);
	}
}

static int app_launchl(launcher_t *launcher, const char *app, ...)
{
	errno_t rc;
	task_id_t id;
	task_wait_t wait;
	va_list ap;
	const char *arg;
	const char **argv;
	const char **argp;
	char *dspec;
	int cnt = 0;
	int i;
	int rv;

	va_start(ap, app);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	argv = calloc(cnt + 4, sizeof(const char *));
	if (argv == NULL)
		return -1;

	task_exit_t texit;
	int retval;

	argp = argv;
	*argp++ = app;

	rv = asprintf(&dspec, "%s?idev=%zu", display_spec,
	    (size_t)launcher->ev_pos_id);
	if (rv < 0) {
		printf("Out of memory.\n");
		return -1;
	}

	/* TODO Might be omitted if default display AND only one seat */
	*argp++ = "-d";
	*argp++ = dspec;

	va_start(ap, app);
	do {
		arg = va_arg(ap, const char *);
		*argp++ = arg;
	} while (arg != NULL);
	va_end(ap);

	*argp++ = NULL;

	printf("%s: Spawning %s", NAME, app);
	for (i = 0; argv[i] != NULL; i++) {
		printf(" %s", argv[i]);
	}
	printf("\n");

	rc = task_spawnv(&id, &wait, app, argv);
	if (rc != EOK) {
		printf("%s: Error spawning %s (%s)\n", NAME, app, str_error(rc));
		return -1;
	}

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL)) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return -1;
	}

	return retval;
}

static void print_syntax(void)
{
	printf("Syntax: %s [-d <display-spec>]\n", NAME);
}

int main(int argc, char *argv[])
{
	int i;
	launcher_t launcher;
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_resource_t *ui_res;
	gfx_bitmap_params_t logo_params;
	gfx_bitmap_t *logo_bmp;
	gfx_context_t *gc;
	gfx_rect_t logo_rect;
	gfx_rect_t rect;
	gfx_coord2_t off;
	const char *dspec = UI_DISPLAY_DEFAULT;
	char *qmark;
	errno_t rc;

	i = 1;
	while (i < argc) {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			dspec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	display_spec = str_dup(dspec);
	if (display_spec == NULL) {
		printf("Out of memory.\n");
		return 1;
	}

	/* Remove additional arguments */
	qmark = str_chr(display_spec, '?');
	if (qmark != NULL)
		*qmark = '\0';

	rc = ui_create(dspec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "Launcher";
	params.placement = ui_wnd_place_top_right;
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 210;
	params.rect.p1.y = 345;

	memset((void *) &launcher, 0, sizeof(launcher));
	launcher.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &launcher);
	launcher.window = window;

	ui_res = ui_window_get_res(window);
	gc = ui_window_get_gc(window);

	rc = decode_tga(gc, (void *) helenos_tga, helenos_tga_size,
	    &logo_bmp, &logo_rect);
	if (rc != EOK) {
		printf("Unable to decode logo.\n");
		return 1;
	}

	gfx_bitmap_params_init(&logo_params);
	logo_params.rect = logo_rect;

	rc = ui_fixed_create(&launcher.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_image_create(ui_res, logo_bmp, &logo_rect, &launcher.image);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	off.x = 6;
	off.y = 32;
	gfx_rect_translate(&off, &logo_rect, &rect);

	/* Adjust for frame width (2 x 1 pixel) */
	rect.p1.x += 2;
	rect.p1.y += 2;
	ui_image_set_rect(launcher.image, &rect);
	ui_image_set_flags(launcher.image, ui_imgf_frame);

	rc = ui_fixed_add(launcher.fixed, ui_image_ctl(launcher.image));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_label_create(ui_res, "Launch application", &launcher.label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	rect.p0.x = 60;
	rect.p0.y = 107;
	rect.p1.x = 160;
	rect.p1.y = 120;
	ui_label_set_rect(launcher.label, &rect);
	ui_label_set_halign(launcher.label, gfx_halign_center);

	rc = ui_fixed_add(launcher.fixed, ui_label_ctl(launcher.label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Navigator */

	rc = ui_pbutton_create(ui_res, "Navigator", &launcher.pb1);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb1, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb1, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb1));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Text Editor */

	rc = ui_pbutton_create(ui_res, "Text Editor", &launcher.pb2);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb2, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130 + 35;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb2, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb2));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Terminal */

	rc = ui_pbutton_create(ui_res, "Terminal", &launcher.pb3);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb3, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130 + 2 * 35;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb3, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb3));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Calculator */

	rc = ui_pbutton_create(ui_res, "Calculator", &launcher.pb4);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb4, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130 + 3 * 35;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb4, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb4));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* UI Demo */

	rc = ui_pbutton_create(ui_res, "UI Demo", &launcher.pb5);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb5, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130 + 4 * 35;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb5, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb5));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* GFX Demo */

	rc = ui_pbutton_create(ui_res, "GFX Demo", &launcher.pb6);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(launcher.pb6, &pbutton_cb, (void *) &launcher);

	rect.p0.x = 15;
	rect.p0.y = 130 + 5 * 35;
	rect.p1.x = 190;
	rect.p1.y = rect.p0.y + 28;
	ui_pbutton_set_rect(launcher.pb6, &rect);

	rc = ui_fixed_add(launcher.fixed, ui_pbutton_ctl(launcher.pb6));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	ui_window_add(window, ui_fixed_ctl(launcher.fixed));

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	ui_run(ui);

	ui_window_destroy(window);
	ui_destroy(ui);

	return 0;
}

/** @}
 */
