/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup aboutos
 * @{
 */
/** @file About HelenOS
 */

#include <errno.h>
#include <gfx/coord.h>
#include <gfximage/tga.h>
#include <macros.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <ui/fixed.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>

#include "aboutos.h"
#include "images.h"

#define NAME  "aboutos"

static void aboutos_wnd_close(ui_window_t *, void *);
static void aboutos_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = aboutos_wnd_close,
	.kbd = aboutos_wnd_kbd
};

static void pb_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t pbutton_cb = {
	.clicked = pb_clicked
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (aboutos)
 */
static void aboutos_wnd_close(ui_window_t *window, void *arg)
{
	aboutos_t *aboutos = (aboutos_t *) arg;

	ui_quit(aboutos->ui);
}

/** About HelenOS window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (ui_prompt_dialog_t *)
 * @param event Keyboard event
 */
static void aboutos_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	aboutos_t *aboutos = (aboutos_t *) arg;

	(void)window;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* Quit */
			ui_quit(aboutos->ui);

		}
	}

	ui_window_def_kbd(window, event);
}

/** Push button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (aboutos)
 */
static void pb_clicked(ui_pbutton_t *pbutton, void *arg)
{
	aboutos_t *aboutos = (aboutos_t *) arg;

	ui_quit(aboutos->ui);
}

static void print_syntax(void)
{
	printf("Syntax: %s [-d <display-spec>]\n", NAME);
}

int main(int argc, char *argv[])
{
	int i;
	aboutos_t aboutos;
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
	const char *dspec = UI_ANY_DEFAULT;
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

	rc = ui_create(dspec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", dspec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "About HelenOS";

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 45;
		params.rect.p1.y = 15;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 350;
		params.rect.p1.y = 275;
	}

	memset((void *) &aboutos, 0, sizeof(aboutos));
	aboutos.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &aboutos);
	aboutos.window = window;

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

	rc = ui_fixed_create(&aboutos.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_image_create(ui_res, logo_bmp, &logo_rect, &aboutos.image);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	off.x = 76;
	off.y = 42;
	gfx_rect_translate(&off, &logo_rect, &rect);

	/* Adjust for frame width (2 x 1 pixel) */
	rect.p1.x += 2;
	rect.p1.y += 2;
	ui_image_set_rect(aboutos.image, &rect);
	ui_image_set_flags(aboutos.image, ui_imgf_frame);

	rc = ui_fixed_add(aboutos.fixed, ui_image_ctl(aboutos.image));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Release label */

	rc = ui_label_create(ui_res, "HelenOS " STRING(HELENOS_RELEASE)
	    " (" STRING(HELENOS_CODENAME) ")", &aboutos.lrelease);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 1;
		rect.p0.y = 5;
		rect.p1.x = 44;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 140;
		rect.p1.x = 340;
		rect.p1.y = 160;
	}

	ui_label_set_rect(aboutos.lrelease, &rect);
	ui_label_set_halign(aboutos.lrelease, gfx_halign_center);

	rc = ui_fixed_add(aboutos.fixed, ui_label_ctl(aboutos.lrelease));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Copyright label */

	rc = ui_label_create(ui_res, STRING(HELENOS_COPYRIGHT), &aboutos.lcopy);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 1;
		rect.p0.y = 6;
		rect.p1.x = 44;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 160;
		rect.p1.x = 340;
		rect.p1.y = 180;
	}

	ui_label_set_rect(aboutos.lcopy, &rect);
	ui_label_set_halign(aboutos.lcopy, gfx_halign_center);

	rc = ui_fixed_add(aboutos.fixed, ui_label_ctl(aboutos.lcopy));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Architecture label */

	rc = ui_label_create(ui_res, "Running on " STRING(UARCH), &aboutos.larch);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 1;
		rect.p0.y = 9;
		rect.p1.x = 44;
		rect.p1.y = 10;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 190;
		rect.p1.x = 340;
		rect.p1.y = 210;
	}

	ui_label_set_rect(aboutos.larch, &rect);
	ui_label_set_halign(aboutos.larch, gfx_halign_center);

	rc = ui_fixed_add(aboutos.fixed, ui_label_ctl(aboutos.larch));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* OK button */

	rc = ui_pbutton_create(ui_res, "OK", &aboutos.pbok);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(aboutos.pbok, &pbutton_cb, (void *) &aboutos);

	if (ui_is_textmode(ui)) {
		rect.p0.x = 17;
		rect.p0.y = 13;
		rect.p1.x = 28;
		rect.p1.y = 14;
	} else {
		rect.p0.x = 125;
		rect.p0.y = 235;
		rect.p1.x = 225;
		rect.p1.y = rect.p0.y + 28;
	}

	ui_pbutton_set_rect(aboutos.pbok, &rect);
	ui_pbutton_set_default(aboutos.pbok, true);

	rc = ui_fixed_add(aboutos.fixed, ui_pbutton_ctl(aboutos.pbok));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	ui_window_add(window, ui_fixed_ctl(aboutos.fixed));

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
