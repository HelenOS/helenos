/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup nav
 * @{
 */
/** @file Navigator.
 *
 * HelenOS file manager.
 */

#include <fibril.h>
#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <ui/fixed.h>
#include <ui/filelist.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "menu.h"
#include "newfile.h"
#include "nav.h"
#include "panel.h"

#define EDITOR_CMD "/app/edit"

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd
};

static void navigator_file_new_file(void *);
static void navigator_file_open(void *);
static void navigator_file_edit(void *);
static void navigator_file_exit(void *);

static nav_menu_cb_t navigator_menu_cb = {
	.file_new_file = navigator_file_new_file,
	.file_open = navigator_file_open,
	.file_edit = navigator_file_edit,
	.file_exit = navigator_file_exit
};

static void navigator_panel_activate_req(void *, panel_t *);
static void navigator_panel_file_open(void *, panel_t *, const char *);

static panel_cb_t navigator_panel_cb = {
	.activate_req = navigator_panel_activate_req,
	.file_open = navigator_panel_file_open
};

static void navigator_progress_babort(progress_dlg_t *, void *);
static void navigator_progress_close(progress_dlg_t *, void *);

progress_dlg_cb_t navigator_progress_cb = {
	.babort = navigator_progress_babort,
	.close = navigator_progress_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (navigator)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	navigator_t *navigator = (navigator_t *) arg;

	ui_quit(navigator->ui);
}

/** Window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (navigator)
 * @param event Keyboard event
 */
static void wnd_kbd(ui_window_t *window, void *arg, kbd_event_t *event)
{
	navigator_t *navigator = (navigator_t *) arg;

	if (event->type == KEY_PRESS &&
	    ((event->mods & KM_ALT) == 0) &&
	    ((event->mods & KM_SHIFT) == 0) &&
	    (event->mods & KM_CTRL) != 0) {
		switch (event->key) {
		case KC_M:
			navigator_new_file_dlg(navigator);
			break;
		case KC_E:
			navigator_file_edit((void *)navigator);
			break;
		case KC_Q:
			ui_quit(navigator->ui);
			break;
		default:
			break;
		}
	}

	if (event->type == KEY_PRESS &&
	    ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0)) {
		switch (event->key) {
		case KC_TAB:
			navigator_switch_panel(navigator);
			break;
		default:
			break;
		}
	}

	ui_window_def_kbd(window, event);
}

/** Create navigator.
 *
 * @param display_spec Display specification
 * @param rnavigator Place to store pointer to new navigator
 * @return EOK on success or ane error code
 */
errno_t navigator_create(const char *display_spec,
    navigator_t **rnavigator)
{
	navigator_t *navigator;
	ui_wnd_params_t params;
	gfx_rect_t rect;
	gfx_rect_t arect;
	gfx_coord_t pw;
	unsigned i;
	errno_t rc;

	navigator = calloc(1, sizeof(navigator_t));
	if (navigator == NULL)
		return ENOMEM;

	rc = ui_create(display_spec, &navigator->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Navigator";
	params.style &= ~ui_wds_decorated;
	params.placement = ui_wnd_place_full_screen;

	rc = ui_window_create(navigator->ui, &params, &navigator->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(navigator->window, &window_cb, (void *) navigator);
	ui_window_get_app_rect(navigator->window, &arect);

	rc = ui_fixed_create(&navigator->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	ui_window_add(navigator->window, ui_fixed_ctl(navigator->fixed));

	rc = nav_menu_create(navigator->window, &navigator->menu);
	if (rc != EOK)
		goto error;

	nav_menu_set_cb(navigator->menu, &navigator_menu_cb,
	    (void *)navigator);

	rc = ui_fixed_add(navigator->fixed, nav_menu_ctl(navigator->menu));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	/* Panel width */
	pw = (arect.p1.x - arect.p0.x) / 2;

	for (i = 0; i < 2; i++) {
		rc = panel_create(navigator->window, i == 0,
		    &navigator->panel[i]);
		if (rc != EOK)
			goto error;

		rect.p0.x = arect.p0.x + pw * i;
		rect.p0.y = arect.p0.y + 1;
		rect.p1.x = arect.p0.x + pw * (i + 1);
		rect.p1.y = arect.p1.y - 1;
		panel_set_rect(navigator->panel[i], &rect);

		panel_set_cb(navigator->panel[i], &navigator_panel_cb,
		    navigator);

		rc = ui_fixed_add(navigator->fixed,
		    panel_ctl(navigator->panel[i]));
		if (rc != EOK) {
			printf("Error adding control to layout.\n");
			goto error;
		}

		rc = panel_read_dir(navigator->panel[i], ".");
		if (rc != EOK) {
			printf("Error reading directory.\n");
			goto error;
		}
	}

	rc = ui_window_paint(navigator->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	*rnavigator = navigator;
	return EOK;
error:
	navigator_destroy(navigator);
	return rc;
}

void navigator_destroy(navigator_t *navigator)
{
	unsigned i;

	for (i = 0; i < 2; i++) {
		if (navigator->panel[i] != NULL) {
			ui_fixed_remove(navigator->fixed,
			    panel_ctl(navigator->panel[i]));
			panel_destroy(navigator->panel[i]);
		}
	}

	if (navigator->menu != NULL) {
		ui_fixed_remove(navigator->fixed, nav_menu_ctl(navigator->menu));
		nav_menu_destroy(navigator->menu);
	}

	if (navigator->window != NULL)
		ui_window_destroy(navigator->window);
	if (navigator->ui != NULL)
		ui_destroy(navigator->ui);
	free(navigator);
}

/** Run navigator on the specified display. */
errno_t navigator_run(const char *display_spec)
{
	navigator_t *navigator;
	errno_t rc;

	rc = navigator_create(display_spec, &navigator);
	if (rc != EOK)
		return rc;

	ui_run(navigator->ui);

	navigator_destroy(navigator);
	return EOK;
}

/** Get the currently active navigator panel.
 *
 * @param navigator Navigator
 * @return Currently active panel
 */
panel_t *navigator_get_active_panel(navigator_t *navigator)
{
	int i;

	for (i = 0; i < navigator_panels; i++) {
		if (panel_is_active(navigator->panel[i]))
			return navigator->panel[i];
	}

	/* This should not happen */
	assert(false);
	return NULL;
}

/** Switch to another navigator panel.
 *
 * Changes the currently active navigator panel to the next panel.
 *
 * @param navigator Navigator
 */
void navigator_switch_panel(navigator_t *navigator)
{
	errno_t rc;

	if (panel_is_active(navigator->panel[0])) {
		rc = panel_activate(navigator->panel[1]);
		if (rc != EOK)
			return;
		panel_deactivate(navigator->panel[0]);
	} else {
		rc = panel_activate(navigator->panel[0]);
		if (rc != EOK)
			return;
		panel_deactivate(navigator->panel[1]);
	}
}

/** Refresh navigator panels.
 *
 * This needs to be called when the disk/directory contents might have
 * changed.
 *
 * @param navigator Navigator
 */
void navigator_refresh_panels(navigator_t *navigator)
{
	errno_t rc;
	unsigned i;

	/* First refresh inactive panel. */

	for (i = 0; i < 2; i++) {
		if (!panel_is_active(navigator->panel[i])) {
			rc = panel_refresh(navigator->panel[i]);
			if (rc != EOK)
				return;
		}
	}

	/*
	 * Refresh active panel last so that working directory is left
	 * to that of the active panel.
	 */

	for (i = 0; i < 2; i++) {
		if (panel_is_active(navigator->panel[i])) {
			rc = panel_refresh(navigator->panel[i]);
			if (rc != EOK)
				return;
		}
	}
}

/** File / New File menu entry selected */
static void navigator_file_new_file(void *arg)
{
	navigator_t *navigator = (navigator_t *)arg;

	navigator_new_file_dlg(navigator);
}

/** File / Open menu entry selected */
static void navigator_file_open(void *arg)
{
	navigator_t *navigator = (navigator_t *)arg;
	panel_t *panel;

	panel = navigator_get_active_panel(navigator);
	ui_file_list_open(panel->flist, ui_file_list_get_cursor(panel->flist));
}

/** Open file in text editor.
 *
 * @param navigator Navigator
 * @param fname File name
 *
 * @return EOK on success or an error code
 */
static errno_t navigator_edit_file(navigator_t *navigator, const char *fname)
{
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	int retval;
	errno_t rc;

	/* Free up and clean console for the child task. */
	rc = ui_suspend(navigator->ui);
	if (rc != EOK)
		return rc;

	rc = task_spawnl(&id, &wait, EDITOR_CMD, EDITOR_CMD, fname, NULL);
	if (rc != EOK)
		goto error;

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL))
		goto error;

	/* Resume UI operation */
	rc = ui_resume(navigator->ui);
	if (rc != EOK)
		return rc;

	navigator_refresh_panels(navigator);
	(void) ui_paint(navigator->ui);
	return EOK;
error:
	(void) ui_resume(navigator->ui);
	(void) ui_paint(navigator->ui);
	return rc;
}

/** Execute file entry.
 *
 * @param navigator Navigator
 * @param fname File name
 *
 * @return EOK on success or an error code
 */
static errno_t navigator_exec_file(navigator_t *navigator, const char *fname)
{
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	int retval;
	errno_t rc;

	/* Free up and clean console for the child task. */
	rc = ui_suspend(navigator->ui);
	if (rc != EOK)
		return rc;

	rc = task_spawnl(&id, &wait, fname, fname, NULL);
	if (rc != EOK)
		goto error;

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL))
		goto error;

	/* Resume UI operation */
	rc = ui_resume(navigator->ui);
	if (rc != EOK)
		return rc;

	navigator_refresh_panels(navigator);

	(void) ui_paint(navigator->ui);
	return EOK;
error:
	(void) ui_resume(navigator->ui);
	(void) ui_paint(navigator->ui);
	return rc;
}

/** Open panel file entry.
 *
 * Perform Open action on a file entry (based on extension).
 *
 * @param navigator Navigator
 * @param fname File name
 *
 * @return EOK on success or an error code
 */
static errno_t navigator_open_file(navigator_t *navigator, const char *fname)
{
	const char *ext;

	ext = str_rchr(fname, '.');
	if (ext != NULL) {
		if (str_casecmp(ext, ".txt") == 0)
			return navigator_edit_file(navigator, fname);
	}

	return navigator_exec_file(navigator, fname);
}

/** File / Edit menu entry selected */
static void navigator_file_edit(void *arg)
{
	navigator_t *navigator = (navigator_t *)arg;
	ui_file_list_entry_t *entry;
	ui_file_list_entry_attr_t attr;
	panel_t *panel;

	panel = navigator_get_active_panel(navigator);
	entry = ui_file_list_get_cursor(panel->flist);
	ui_file_list_entry_get_attr(entry, &attr);

	(void)navigator_edit_file(navigator, attr.name);
}

/** File / Exit menu entry selected */
static void navigator_file_exit(void *arg)
{
	navigator_t *navigator = (navigator_t *)arg;

	ui_quit(navigator->ui);
}

/** Panel callback requesting panel activation.
 *
 * @param arg Argument (navigator_t *)
 * @param panel Panel
 */
void navigator_panel_activate_req(void *arg, panel_t *panel)
{
	navigator_t *navigator = (navigator_t *)arg;

	if (!panel_is_active(panel))
		navigator_switch_panel(navigator);
}

/** Panel callback requesting file open.
 *
 * @param arg Argument (navigator_t *)
 * @param panel Panel
 * @param fname File name
 */
void navigator_panel_file_open(void *arg, panel_t *panel, const char *fname)
{
	navigator_t *navigator = (navigator_t *)arg;

	(void)panel;
	navigator_open_file(navigator, fname);
}

/** Wrapper fibril function for worker function.
 *
 * This is the main fibril function for the worker fibril. It executes
 * the worker function, then clears worker FID to indicate the worker
 * is finished.
 *
 * @param arg Argument (navigator_worker_job_t *)
 * @return EOK
 */
static errno_t navigator_worker_func(void *arg)
{
	navigator_worker_job_t *job = (navigator_worker_job_t *)arg;

	job->wfunc(job->arg);
	job->navigator->worker_fid = 0;
	free(job);
	return EOK;
}

/** Start long-time work in a worker fibril.
 *
 * Actions which can take time (file operations) cannot block the main UI
 * fibril. This function will start an action in the worker fibril, i.e.,
 * in the background. At the same time the caller should create a modal
 * progress dialog that will be shown until the work is completed.
 *
 * (Only a single worker can execute at any given time).
 *
 * @param nav Navigator
 * @param wfunc Worker main function
 * @param arg Argument to worker function
 *
 * @return EOK on success or an error code
 */
errno_t navigator_worker_start(navigator_t *nav, void (*wfunc)(void *),
    void *arg)
{
	navigator_worker_job_t *job;

	if (nav->worker_fid != 0)
		return EBUSY;

	job = calloc(1, sizeof(navigator_worker_job_t));
	if (job == NULL)
		return ENOMEM;

	job->navigator = nav;
	job->wfunc = wfunc;
	job->arg = arg;

	nav->worker_fid = fibril_create(navigator_worker_func, (void *)job);
	if (nav->worker_fid == 0) {
		free(job);
		return ENOMEM;
	}

	fibril_add_ready(nav->worker_fid);
	return EOK;
}

/** Abort button pressed in progress dialog.
 *
 * @param dlg Progress dialog
 * @param arg Argument (navigator_t *)
 */
static void navigator_progress_babort(progress_dlg_t *dlg, void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	(void)dlg;
	nav->abort_op = true;
}

/** Progress dialog closed,
 *
 * @param dlg Progress dialog
 * @param arg Argument (navigator_t *)
 */
static void navigator_progress_close(progress_dlg_t *dlg, void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	(void)dlg;
	nav->abort_op = true;
}

/** @}
 */
