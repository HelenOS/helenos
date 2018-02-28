/*
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

/** @addtogroup vdemo
 * @{
 */
/** @file
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <io/pixel.h>
#include <task.h>

#include <window.h>
#include <grid.h>
#include <button.h>
#include <label.h>

#define NAME "vdemo"

typedef struct my_label {
	label_t label;
	slot_t confirm;
	slot_t cancel;
} my_label_t;

static void deinit_my_label(my_label_t *lbl)
{
	deinit_label(&lbl->label);
}

static void my_label_destroy(widget_t *widget)
{
	my_label_t *lbl = (my_label_t *) widget;

	deinit_my_label(lbl);

	free(lbl);
}

static void on_confirm(widget_t *widget, void *data)
{
	my_label_t *lbl = (my_label_t *) widget;
	const char *confirmed = "Confirmed";
	lbl->label.rewrite(&lbl->label.widget, (void *) confirmed);
}

static void on_cancel(widget_t *widget, void *data)
{
	my_label_t *lbl = (my_label_t *) widget;
	const char *cancelled = "Cancelled";
	lbl->label.rewrite(&lbl->label.widget, (void *) cancelled);
}

static bool init_my_label(my_label_t *lbl, widget_t *parent,
    const char *caption, uint16_t points, pixel_t background, pixel_t foreground)
{
	lbl->confirm = on_confirm;
	lbl->cancel = on_cancel;
	bool initialized = init_label(&lbl->label, parent, NULL, caption,
	    points, background, foreground);
	lbl->label.widget.destroy = my_label_destroy;
	return initialized;
}

static my_label_t *create_my_label(widget_t *parent,
    const char *caption, uint16_t points, pixel_t background, pixel_t foreground)
{
	my_label_t *lbl = (my_label_t *) malloc(sizeof(my_label_t));
	if (!lbl) {
		return NULL;
	}

	if (init_my_label(lbl, parent, caption, points, background, foreground)) {
		return lbl;
	} else {
		free(lbl);
		return NULL;
	}
}

int main(int argc, char *argv[])
{
	if (argc >= 2) {
		window_t *main_window = window_open(argv[1], NULL,
		    WINDOW_MAIN | WINDOW_DECORATED | WINDOW_RESIZEABLE, "vdemo");
		if (!main_window) {
			printf("Cannot open main window.\n");
			return 1;
		}

		pixel_t grd_bg = PIXEL(255, 240, 240, 240);

		pixel_t btn_bg = PIXEL(255, 240, 240, 240);
		pixel_t btn_fg = PIXEL(255, 186, 186, 186);
		pixel_t btn_text = PIXEL(255, 0, 0, 0);

		pixel_t lbl_bg = PIXEL(255, 240, 240, 240);
		pixel_t lbl_text = PIXEL(255, 0, 0, 0);

		my_label_t *lbl_action = create_my_label(NULL, "Hello there!", 16,
		    lbl_bg, lbl_text);
		button_t *btn_confirm = create_button(NULL, NULL, "Confirm", 16,
		    btn_bg, btn_fg, btn_text);
		button_t *btn_cancel = create_button(NULL, NULL, "Cancel", 16,
		    btn_bg, btn_fg, btn_text);
		grid_t *grid = create_grid(window_root(main_window), NULL, 2, 2,
		    grd_bg);
		if (!lbl_action || !btn_confirm || !btn_cancel || !grid) {
			window_close(main_window);
			printf("Cannot create widgets.\n");
			return 1;
		}

		sig_connect(
		    &btn_confirm->clicked,
		    &lbl_action->label.widget,
		    lbl_action->confirm);
		sig_connect(
		    &btn_cancel->clicked,
		    &lbl_action->label.widget,
		    lbl_action->cancel);

		grid->add(grid, &lbl_action->label.widget, 0, 0, 2, 1);
		grid->add(grid, &btn_confirm->widget, 0, 1, 1, 1);
		grid->add(grid, &btn_cancel->widget, 1, 1, 1, 1);
		window_resize(main_window, 0, 0, 200, 76,
		    WINDOW_PLACEMENT_CENTER);

		window_exec(main_window);
		task_retval(0);
		async_manager();
		return 1;
	} else {
		printf("Compositor server not specified.\n");
		return 1;
	}
}

/** @}
 */
