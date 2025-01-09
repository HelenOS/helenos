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

/** @addtogroup libui
 * @{
 */
/**
 * @file File dialog
 */

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/entry.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/filedialog.h>
#include <ui/filelist.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/filedialog.h"

static void ui_file_dialog_wnd_close(ui_window_t *, void *);
static void ui_file_dialog_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t ui_file_dialog_wnd_cb = {
	.close = ui_file_dialog_wnd_close,
	.kbd = ui_file_dialog_wnd_kbd
};

static void ui_file_dialog_flist_activate_req(ui_file_list_t *, void *);
static void ui_file_dialog_flist_selected(ui_file_list_t *, void *,
    const char *);

ui_file_list_cb_t ui_file_dialog_flist_cb = {
	.activate_req = ui_file_dialog_flist_activate_req,
	.selected = ui_file_dialog_flist_selected
};

static void ui_file_dialog_bok_clicked(ui_pbutton_t *, void *);
static void ui_file_dialog_bcancel_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t ui_file_dialog_bok_cb = {
	.clicked = ui_file_dialog_bok_clicked
};

ui_pbutton_cb_t ui_file_dialog_bcancel_cb = {
	.clicked = ui_file_dialog_bcancel_clicked
};

/** Initialize file dialog parameters structure.
 *
 * File dialog parameters structure must always be initialized using
 * this function first.
 *
 * @param params File dialog parameters structure
 */
void ui_file_dialog_params_init(ui_file_dialog_params_t *params)
{
	memset(params, 0, sizeof(ui_file_dialog_params_t));
	params->ifname = "";
}

/** Create new file dialog.
 *
 * @param ui User interface
 * @param params File dialog parameters
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t ui_file_dialog_create(ui_t *ui, ui_file_dialog_params_t *params,
    ui_file_dialog_t **rdialog)
{
	errno_t rc;
	ui_file_dialog_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_entry_t *entry = NULL;
	ui_file_list_t *flist = NULL;
	ui_pbutton_t *bok = NULL;
	ui_pbutton_t *bcancel = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;

	dialog = calloc(1, sizeof(ui_file_dialog_t));
	if (dialog == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ui_wnd_params_init(&wparams);
	wparams.caption = params->caption;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 40;
		wparams.rect.p1.y = 20;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 300;
		wparams.rect.p1.y = 335;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &ui_file_dialog_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	rc = ui_label_create(ui_res, "File Name:", &label);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 2;
		rect.p1.x = 17;
		rect.p1.y = 3;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 190;
		rect.p1.y = 50;
	}

	ui_label_set_rect(label, &rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	rc = ui_entry_create(window, params->ifname, &entry);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 3;
		rect.p1.x = 37;
		rect.p1.y = 4;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 55;
		rect.p1.x = 290;
		rect.p1.y = 80;
	}

	ui_entry_set_rect(entry, &rect);

	rc = ui_fixed_add(fixed, ui_entry_ctl(entry));
	if (rc != EOK)
		goto error;

	ui_entry_activate(entry);

	/* Select all */
	ui_entry_seek_start(entry, false);
	ui_entry_seek_end(entry, true);

	dialog->ename = entry;
	entry = NULL;

	/* Files label */
	rc = ui_label_create(ui_res, "Files:", &label);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 5;
		rect.p1.x = 17;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 90;
		rect.p1.x = 190;
		rect.p1.y = 105;
	}

	ui_label_set_rect(label, &rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	/* File list */

	rc = ui_file_list_create(window, false, &flist);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 6;
		rect.p1.x = 37;
		rect.p1.y = 16;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 110;
		rect.p1.x = 290;
		rect.p1.y = 280;
	}

	ui_file_list_set_rect(flist, &rect);
	ui_file_list_set_cb(flist, &ui_file_dialog_flist_cb, dialog);

	rc = ui_fixed_add(fixed, ui_file_list_ctl(flist));
	if (rc != EOK)
		goto error;

	dialog->flist = flist;
	flist = NULL;

	rc = ui_file_list_read_dir(dialog->flist, ".");
	if (rc != EOK)
		goto error;

	/* OK button */

	rc = ui_pbutton_create(ui_res, "OK", &bok);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bok, &ui_file_dialog_bok_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 10;
		rect.p0.y = 17;
		rect.p1.x = 20;
		rect.p1.y = 18;
	} else {
		rect.p0.x = 55;
		rect.p0.y = 290;
		rect.p1.x = 145;
		rect.p1.y = 318;
	}

	ui_pbutton_set_rect(bok, &rect);

	ui_pbutton_set_default(bok, true);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(bok));
	if (rc != EOK)
		goto error;

	dialog->bok = bok;
	bok = NULL;

	/* Cancel button */

	rc = ui_pbutton_create(ui_res, "Cancel", &bcancel);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bcancel, &ui_file_dialog_bcancel_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 22;
		rect.p0.y = 17;
		rect.p1.x = 32;
		rect.p1.y = 18;
	} else {
		rect.p0.x = 155;
		rect.p0.y = 290;
		rect.p1.x = 245;
		rect.p1.y = 318;
	}

	ui_pbutton_set_rect(bcancel, &rect);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(bcancel));
	if (rc != EOK)
		goto error;

	dialog->bcancel = bcancel;
	bcancel = NULL;

	ui_window_add(window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(window);
	if (rc != EOK)
		goto error;

	dialog->window = window;
	*rdialog = dialog;
	return EOK;
error:
	if (entry != NULL)
		ui_entry_destroy(entry);
	if (flist != NULL)
		ui_file_list_destroy(flist);
	if (bok != NULL)
		ui_pbutton_destroy(bok);
	if (bcancel != NULL)
		ui_pbutton_destroy(bcancel);
	if (label != NULL)
		ui_label_destroy(label);
	if (fixed != NULL)
		ui_fixed_destroy(fixed);
	if (window != NULL)
		ui_window_destroy(window);
	if (dialog != NULL)
		free(dialog);
	return rc;
}

/** Destroy file dialog.
 *
 * @param dialog File dialog or @c NULL
 */
void ui_file_dialog_destroy(ui_file_dialog_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set mesage dialog callback.
 *
 * @param dialog File dialog
 * @param cb File dialog callbacks
 * @param arg Callback argument
 */
void ui_file_dialog_set_cb(ui_file_dialog_t *dialog, ui_file_dialog_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** File dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (ui_file_dialog_t *)
 */
static void ui_file_dialog_wnd_close(ui_window_t *window, void *arg)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;

	if (dialog->cb != NULL && dialog->cb->close != NULL)
		dialog->cb->close(dialog, dialog->arg);
}

/** File dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (ui_file_dialog_t *)
 * @param event Keyboard event
 */
static void ui_file_dialog_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;
	const char *fname;
	ui_evclaim_t claim;

	claim = ui_window_def_kbd(window, event);
	if (claim == ui_claimed)
		return;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* Confirm */
			if (dialog->cb != NULL && dialog->cb->bok != NULL) {
				fname = ui_entry_get_text(dialog->ename);
				dialog->cb->bok(dialog, dialog->arg, fname);
				return;
			}
		} else if (event->key == KC_ESCAPE) {
			/* Cancel */
			if (dialog->cb != NULL && dialog->cb->bcancel != NULL) {
				dialog->cb->bcancel(dialog, dialog->arg);
				return;
			}
		}
	}
}

static void ui_file_dialog_flist_activate_req(ui_file_list_t *flist, void *arg)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;

	ui_file_list_activate(dialog->flist);
	ui_entry_deactivate(dialog->ename);
}

static void ui_file_dialog_flist_selected(ui_file_list_t *flist, void *arg,
    const char *fname)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;

	dialog->cb->bok(dialog, dialog->arg, fname);
}

/** File dialog OK button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_file_dialog_t *)
 */
static void ui_file_dialog_bok_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;
	const char *fname;

	if (dialog->cb != NULL && dialog->cb->bok != NULL) {
		fname = ui_entry_get_text(dialog->ename);
		dialog->cb->bok(dialog, dialog->arg, fname);
	}
}

/** File dialog cancel button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_file_dialog_t *)
 */
static void ui_file_dialog_bcancel_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;

	if (dialog->cb != NULL && dialog->cb->bcancel != NULL)
		dialog->cb->bcancel(dialog, dialog->arg);
}

/** @}
 */
