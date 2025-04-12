/*
 * Copyright (c) 2025 Wayne Michael Thornton (WMT) <wmthornton-dev@outlook.com>
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

/** @addtogroup clock_widget
 * @{
 */
/** @file Clock widget application (in UI)
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <str.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/entry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <device/clock_dev.h>
#include <loc.h>
#include <time.h>
#include "clock_widget.h"

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (clock_widget)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	clock_widget_t *clock_widget = (clock_widget_t *) arg;
	ui_quit(clock_widget->ui);
}

/** Run Clock Widget on display server. */
static errno_t clock_widget(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	clock_widget_t clock_widget;
	errno_t rc;

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "Clock Widget";
	params.placement = ui_wnd_place_center;

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

	memset((void *) &clock_widget, 0, sizeof(clock_widget));
	clock_widget.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &clock_widget);
	clock_widget.window = window;

	rc = ui_fixed_create(&clock_widget.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	ui_run(ui);

	ui_window_destroy(window);
	ui_destroy(ui);

	return EOK;
}

static void print_syntax(void)
{
	printf("Syntax: clock_widget [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
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

	rc = clock_widget(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
