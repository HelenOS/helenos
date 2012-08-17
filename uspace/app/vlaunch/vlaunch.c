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

#include <bool.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <io/pixel.h>
#include <task.h>
#include <str.h>
#include <str_error.h>

#include <window.h>
#include <grid.h>
#include <button.h>
#include <label.h>

#define NAME "vlaunch"

static char *winreg = NULL;

static int app_launch(const char *app)
{
	int rc;
	printf("%s: Spawning %s %s \n", NAME, app, winreg);

	task_id_t id;
	task_exit_t texit;
	int retval;
	rc = task_spawnl(&id, app, app, winreg, NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s %s (%s)\n", NAME, app,
		    winreg, str_error(rc));
		return -1;
	}
	rc = task_wait(id, &texit, &retval);
	if (rc != EOK || texit != TASK_EXIT_NORMAL) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return -1;
	}

	return retval;
}

static void on_vterm(widget_t *widget, void *data)
{
	app_launch("/app/vterm");
}

static void on_vdemo(widget_t *widget, void *data)
{
	app_launch("/app/vdemo");
}

static void on_vlaunch(widget_t *widget, void *data)
{
	app_launch("/app/vlaunch");
}

int main(int argc, char *argv[])
{
	if (argc >= 2) {
		printf("Compositor server not specified.\n");
		return 1;
	}
	
	winreg = argv[1];
	window_t *main_window = window_open(argv[1], true, true, "vlaunch");
	if (!main_window) {
		printf("Cannot open main window.\n");
		return 1;
	}
	
	pixel_t grd_bg = PIXEL(255, 240, 240, 240);
	pixel_t btn_bg = PIXEL(255, 0, 0, 0);
	pixel_t btn_fg = PIXEL(255, 240, 240, 240);
	pixel_t lbl_bg = PIXEL(255, 240, 240, 240);
	pixel_t lbl_fg = PIXEL(255, 0, 0, 0);
	
	label_t *lbl_caption = create_label(NULL, "Launch application:", 16,
	    lbl_bg, lbl_fg);
	button_t *btn_vterm = create_button(NULL, "vterm", 16, btn_bg,
	    btn_fg);
	button_t *btn_vdemo = create_button(NULL, "vdemo", 16, btn_bg,
	    btn_fg);
	button_t *btn_vlaunch = create_button(NULL, "vlaunch", 16, btn_bg,
	    btn_fg);
	grid_t *grid = create_grid(window_root(main_window), 4, 1, grd_bg);
	
	if ((!lbl_caption) || (!btn_vterm) || (!btn_vdemo) ||
	    (!btn_vlaunch) || (!grid)) {
		window_close(main_window);
		printf("Cannot create widgets.\n");
		return 1;
	}
	
	sig_connect(&btn_vterm->clicked, NULL, on_vterm);
	sig_connect(&btn_vdemo->clicked, NULL, on_vdemo);
	sig_connect(&btn_vlaunch->clicked, NULL, on_vlaunch);
	
	grid->add(grid, &lbl_caption->widget, 0, 0, 1, 1);
	grid->add(grid, &btn_vterm->widget, 1, 0, 1, 1);
	grid->add(grid, &btn_vdemo->widget, 2, 0, 1, 1);
	grid->add(grid, &btn_vlaunch->widget, 3, 0, 1, 1);
	
	window_resize(main_window, 180, 130);
	window_exec(main_window);
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
