/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup shutdown-dlg
 * @{
 */
/** @file Shutdown dialog
 */

#include <gfx/coord.h>
#include <gfx/render.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <system.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "shutdown-dlg.h"

static void wnd_close(ui_window_t *, void *);
static errno_t bg_wnd_paint(ui_window_t *, void *);

static ui_window_cb_t bg_window_cb = {
	.close = wnd_close,
	.paint = bg_wnd_paint
};

static ui_window_cb_t progress_window_cb = {
	.close = wnd_close
};

static void sd_shutdown_complete(void *);
static void sd_shutdown_failed(void *);

static system_cb_t sd_system_cb = {
	.shutdown_complete = sd_shutdown_complete,
	.shutdown_failed = sd_shutdown_failed
};

/** System shutdown complete.
 *
 * @param arg Argument (shutdown_dlg_t *)
 */
static void sd_shutdown_complete(void *arg)
{
	shutdown_dlg_t *sddlg = (shutdown_dlg_t *)arg;

	ui_lock(sddlg->ui);
	(void)ui_label_set_text(sddlg->progress->label,
	    "Shutdown complete. It is now safe to remove power.");
	(void)ui_window_paint(sddlg->progress->window);
	ui_unlock(sddlg->ui);
}

/** System shutdown failed.
 *
 * @param arg Argument (shutdown_dlg_t *)
 */
static void sd_shutdown_failed(void *arg)
{
	shutdown_dlg_t *sddlg = (shutdown_dlg_t *)arg;

	ui_lock(sddlg->ui);
	(void)ui_label_set_text(sddlg->progress->label, "Shutdown failed.");
	(void)ui_window_paint(sddlg->progress->window);
	ui_unlock(sddlg->ui);
}

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (shutdown_dlg_t *)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	(void)window;
	(void)arg;
}

/** Paint background window.
 *
 * @param window Window
 * @param arg Argument (shutdown_dlg_t *)
 */
static errno_t bg_wnd_paint(ui_window_t *window, void *arg)
{
	shutdown_dlg_t *sddlg = (shutdown_dlg_t *)arg;
	gfx_rect_t app_rect;
	gfx_context_t *gc;
	errno_t rc;

	gc = ui_window_get_gc(window);

	rc = gfx_set_color(gc, sddlg->bg_color);
	if (rc != EOK)
		return rc;

	ui_window_get_app_rect(window, &app_rect);

	rc = gfx_fill_rect(gc, &app_rect);
	if (rc != EOK)
		return rc;

	rc = gfx_update(gc);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Create shutdown progress window.
 *
 * @param sddlg Shutdown dialog
 * @param rprogress Place to store pointer to new progress window
 * @return EOK on success or an error code
 */
static errno_t shutdown_progress_create(shutdown_dlg_t *sddlg,
    shutdown_progress_t **rprogress)
{
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	shutdown_progress_t *progress;
	ui_fixed_t *fixed = NULL;
	errno_t rc;

	ui_wnd_params_init(&params);
	params.caption = "Shut down";
	params.style &= ~ui_wds_titlebar;
	params.flags |= ui_wndf_topmost;
	params.placement = ui_wnd_place_center;
	if (ui_is_textmode(sddlg->ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 24;
		params.rect.p1.y = 5;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 450;
		params.rect.p1.y = 60;
	}

	progress = calloc(1, sizeof(shutdown_progress_t));
	if (progress == NULL) {
		rc = ENOMEM;
		printf("Out of memory.\n");
		goto error;
	}

	rc = ui_window_create(sddlg->ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &progress_window_cb, (void *) &sddlg);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	rc = ui_label_create(ui_res, "The system is shutting down...",
	    &progress->label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	ui_window_get_app_rect(window, &rect);
	ui_label_set_rect(progress->label, &rect);
	ui_label_set_halign(progress->label, gfx_halign_center);
	ui_label_set_valign(progress->label, gfx_valign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(progress->label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		ui_label_destroy(progress->label);
		progress->label = NULL;
		goto error;
	}

	ui_window_add(window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	progress->window = window;
	progress->fixed = fixed;
	*rprogress = progress;
	return EOK;
error:
	if (progress != NULL && progress->fixed != NULL)
		ui_fixed_destroy(progress->fixed);
	if (window != NULL)
		ui_window_destroy(window);
	if (progress != NULL)
		free(progress);
	return rc;
}

/** Destroy shutdown progress window.
 *
 * @param sddlg Shutdown dialog
 * @param rprogress Place to store pointer to new progress window
 * @return EOK on success or an error code
 */
static void shutdown_progress_destroy(shutdown_progress_t *progress)
{
	ui_window_destroy(progress->window);
	free(progress);
}

/** Run shutdown dialog on display. */
static errno_t shutdown_dlg(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	shutdown_dlg_t sddlg;
	errno_t rc;

	memset((void *) &sddlg, 0, sizeof(sddlg));

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Shut down background";
	params.style &= ~ui_wds_decorated;
	params.placement = ui_wnd_place_full_screen;
	params.flags |= ui_wndf_topmost | ui_wndf_nofocus;
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 24;
		params.rect.p1.y = 5;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 300;
		params.rect.p1.y = 60;
	}

	sddlg.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &bg_window_cb, (void *) &sddlg);
	sddlg.bgwindow = window;

	rc = gfx_color_new_rgb_i16(0x8000, 0xc800, 0xffff, &sddlg.bg_color);
	if (rc != EOK) {
		printf("Error allocating color.\n");
		goto error;
	}

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	rc = shutdown_progress_create(&sddlg, &sddlg.progress);
	if (rc != EOK)
		return rc;

	rc = system_open(SYSTEM_DEFAULT, &sd_system_cb, &sddlg, &sddlg.system);
	if (rc != EOK) {
		printf("Failed opening system control service.\n");
		goto error;
	}

	rc = system_shutdown(sddlg.system);
	if (rc != EOK) {
		printf("Failed requesting system shutdown.\n");
		goto error;
	}

	ui_run(ui);

	shutdown_progress_destroy(sddlg.progress);
	ui_window_destroy(window);
	system_close(sddlg.system);
	gfx_color_delete(sddlg.bg_color);
	ui_destroy(ui);

	return EOK;
error:
	if (sddlg.system != NULL)
		system_close(sddlg.system);
	if (sddlg.bg_color != NULL)
		gfx_color_delete(sddlg.bg_color);
	if (window != NULL)
		ui_window_destroy(window);
	if (ui != NULL)
		ui_destroy(ui);
	return rc;
}

static void print_syntax(void)
{
	printf("Syntax: shutdown-dlg [-d <display-spec>]\n");
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

	rc = shutdown_dlg(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
