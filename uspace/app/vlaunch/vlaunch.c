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

/** @addtogroup vlaunch
 * @{
 */
/** @file
 */

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <task.h>
#include <str_error.h>

#include <window.h>
#include <grid.h>
#include <button.h>
#include <label.h>
#include <canvas.h>

#include <surface.h>
#include <source.h>
#include <drawctx.h>
#include <codec/tga.h>

#include "images.h"

#define NAME  "vlaunch"

#define LOGO_WIDTH   196
#define LOGO_HEIGHT  66

static char *winreg = NULL;

static int app_launch(const char *app)
{
	printf("%s: Spawning %s %s \n", NAME, app, winreg);
	
	task_id_t id;
	task_wait_t wait;
	errno_t rc = task_spawnl(&id, &wait, app, app, winreg, NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s %s (%s)\n", NAME, app,
		    winreg, str_error(rc));
		return -1;
	}
	
	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL)) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return -1;
	}
	
	return retval;
}

static void on_btn_click(widget_t *widget, void *data)
{
	const char *app = (const char *) widget_get_data(widget);
	app_launch(app);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Compositor server not specified.\n");
		return 1;
	}
	
	surface_t *logo = decode_tga((void *) helenos_tga, helenos_tga_size, 0);
	if (!logo) {
		printf("Unable to decode logo.\n");
		return 1;
	}
	
	winreg = argv[1];
	window_t *main_window = window_open(argv[1], NULL,
	    WINDOW_MAIN | WINDOW_DECORATED | WINDOW_RESIZEABLE, "vlaunch");
	if (!main_window) {
		printf("Cannot open main window.\n");
		return 1;
	}
	
	pixel_t grd_bg = PIXEL(255, 255, 255, 255);
	
	pixel_t btn_bg = PIXEL(255, 255, 255, 255);
	pixel_t btn_fg = PIXEL(255, 186, 186, 186);
	pixel_t btn_text = PIXEL(255, 0, 0, 0);
	
	pixel_t lbl_bg = PIXEL(255, 255, 255, 255);
	pixel_t lbl_text = PIXEL(255, 0, 0, 0);
	
	canvas_t *logo_canvas = create_canvas(NULL, NULL, LOGO_WIDTH, LOGO_HEIGHT,
	    logo);
	label_t *lbl_caption = create_label(NULL, NULL, "Launch application:",
	    16, lbl_bg, lbl_text);
	button_t *btn_vterm = create_button(NULL, "/app/vterm", "vterm",
	    16, btn_bg, btn_fg, btn_text);
	button_t *btn_vcalc = create_button(NULL, "/app/vcalc", "vcalc",
	    16, btn_bg, btn_fg, btn_text);
	button_t *btn_vdemo = create_button(NULL, "/app/vdemo", "vdemo",
	    16, btn_bg, btn_fg, btn_text);
	button_t *btn_vlaunch = create_button(NULL, "/app/vlaunch", "vlaunch",
	    16, btn_bg, btn_fg, btn_text);
	grid_t *grid = create_grid(window_root(main_window), NULL, 1, 6, grd_bg);
	
	if ((!logo_canvas) || (!lbl_caption) || (!btn_vterm) ||
	    (!btn_vcalc) || (!btn_vdemo) || (!btn_vlaunch) || (!grid)) {
		window_close(main_window);
		printf("Cannot create widgets.\n");
		return 1;
	}
	
	sig_connect(&btn_vterm->clicked, &btn_vterm->widget, on_btn_click);
	sig_connect(&btn_vcalc->clicked, &btn_vcalc->widget, on_btn_click);
	sig_connect(&btn_vdemo->clicked, &btn_vdemo->widget, on_btn_click);
	sig_connect(&btn_vlaunch->clicked, &btn_vlaunch->widget, on_btn_click);
	
	grid->add(grid, &logo_canvas->widget, 0, 0, 1, 1);
	grid->add(grid, &lbl_caption->widget, 0, 1, 1, 1);
	grid->add(grid, &btn_vterm->widget, 0, 2, 1, 1);
	grid->add(grid, &btn_vcalc->widget, 0, 3, 1, 1);
	grid->add(grid, &btn_vdemo->widget, 0, 4, 1, 1);
	grid->add(grid, &btn_vlaunch->widget, 0, 5, 1, 1);
	
	window_resize(main_window, 0, 0, 210, 164 + LOGO_HEIGHT,
	    WINDOW_PLACEMENT_RIGHT | WINDOW_PLACEMENT_TOP);
	window_exec(main_window);
	
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
