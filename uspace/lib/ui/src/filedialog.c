/*
 * Copyright (c) 2026 Jiri Svoboda
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
#include <str.h>
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

static void ui_file_dialog_bok(ui_file_dialog_t *, const char *);

static void ui_file_dialog_wnd_resize(ui_window_t *, void *);
static void ui_file_dialog_wnd_close(ui_window_t *, void *);
static void ui_file_dialog_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t ui_file_dialog_wnd_cb = {
	.resize = ui_file_dialog_wnd_resize,
	.close = ui_file_dialog_wnd_close,
	.kbd = ui_file_dialog_wnd_kbd
};

static void ui_file_dialog_flist_activate_req(ui_file_list_t *, void *);
static void ui_file_dialog_flist_selected(ui_file_list_t *, void *,
    const char *);
static void ui_file_dialog_flist_dir_selected(ui_file_list_t *, void *,
    const char *);

ui_file_list_cb_t ui_file_dialog_flist_cb = {
	.activate_req = ui_file_dialog_flist_activate_req,
	.selected = ui_file_dialog_flist_selected,
	.dir_selected = ui_file_dialog_flist_dir_selected
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

/** Compute file dialog geometry.
 *
 * @param ui User interface
 * @param wrect Window interior rectangle
 * @param geom Place to store geometry
 */
static void ui_file_dialog_get_geom(ui_t *ui, gfx_rect_t *wrect,
    ui_file_dialog_geom_t *geom)
{
	gfx_coord_t cx;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->fname_label_rect.p0.x = 3;
		geom->fname_label_rect.p0.y = 2;
		geom->fname_label_rect.p1.x = 17;
		geom->fname_label_rect.p1.y = 3;
	} else {
		geom->fname_label_rect.p0.x = 10;
		geom->fname_label_rect.p0.y = 35;
		geom->fname_label_rect.p1.x = 190;
		geom->fname_label_rect.p1.y = 50;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->fname_entry_rect.p0.x = 3;
		geom->fname_entry_rect.p0.y = 3;
		geom->fname_entry_rect.p1.x = wrect->p1.x - 3;
		geom->fname_entry_rect.p1.y = 4;
	} else {
		geom->fname_entry_rect.p0.x = 10;
		geom->fname_entry_rect.p0.y = 55;
		geom->fname_entry_rect.p1.x = wrect->p1.x - 10;
		geom->fname_entry_rect.p1.y = 80;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->files_label_rect.p0.x = 3;
		geom->files_label_rect.p0.y = 5;
		geom->files_label_rect.p1.x = 17;
		geom->files_label_rect.p1.y = 6;
	} else {
		geom->files_label_rect.p0.x = 10;
		geom->files_label_rect.p0.y = 90;
		geom->files_label_rect.p1.x = 190;
		geom->files_label_rect.p1.y = 105;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->flist_rect.p0.x = 3;
		geom->flist_rect.p0.y = 6;
		geom->flist_rect.p1.x = wrect->p1.x - 3;
		geom->flist_rect.p1.y = wrect->p1.y - 6;
	} else {
		geom->flist_rect.p0.x = 10;
		geom->flist_rect.p0.y = 110;
		geom->flist_rect.p1.x = wrect->p1.x - 10;
		geom->flist_rect.p1.y = wrect->p1.y - 105;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->dir_label_rect.p0.x = 3;
		geom->dir_label_rect.p0.y = wrect->p1.y - 5;
		geom->dir_label_rect.p1.x = 17;
		geom->dir_label_rect.p1.y = wrect->p1.y - 4;
	} else {
		geom->dir_label_rect.p0.x = 10;
		geom->dir_label_rect.p0.y = wrect->p1.y - 95;
		geom->dir_label_rect.p1.x = 190;
		geom->dir_label_rect.p1.y = wrect->p1.y - 80;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->dir_entry_rect.p0.x = 3;
		geom->dir_entry_rect.p0.y = wrect->p1.y - 4;
		geom->dir_entry_rect.p1.x = wrect->p1.x - 3;
		geom->dir_entry_rect.p1.y = wrect->p1.y - 3;
	} else {
		geom->dir_entry_rect.p0.x = 10;
		geom->dir_entry_rect.p0.y = wrect->p1.y - 75;
		geom->dir_entry_rect.p1.x = wrect->p1.x - 10;
		geom->dir_entry_rect.p1.y = wrect->p1.y - 50;
	}

	cx = (wrect->p0.x + wrect->p1.x) / 2;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->bok_rect.p0.x = cx - 10;
		geom->bok_rect.p0.y = wrect->p1.y - 2;
		geom->bok_rect.p1.x = cx;
		geom->bok_rect.p1.y = wrect->p1.y - 1;
	} else {
		geom->bok_rect.p0.x = cx - 95;
		geom->bok_rect.p0.y = wrect->p1.y - 40;
		geom->bok_rect.p1.x = cx - 5;
		geom->bok_rect.p1.y = wrect->p1.y - 12;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		geom->bcancel_rect.p0.x = cx + 2;
		geom->bcancel_rect.p0.y = wrect->p1.y - 2;
		geom->bcancel_rect.p1.x = cx + 12;
		geom->bcancel_rect.p1.y = wrect->p1.y - 1;
	} else {
		geom->bcancel_rect.p0.x = cx + 5;
		geom->bcancel_rect.p0.y = wrect->p1.y - 40;
		geom->bcancel_rect.p1.x = cx + 95;
		geom->bcancel_rect.p1.y = wrect->p1.y - 12;
	}
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
	char *dirname = NULL;
	ui_file_list_t *flist = NULL;
	ui_pbutton_t *bok = NULL;
	ui_pbutton_t *bcancel = NULL;
	ui_file_dialog_geom_t geom;
	gfx_rect_t arect;
	ui_resource_t *ui_res;

	dialog = calloc(1, sizeof(ui_file_dialog_t));
	if (dialog == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ui_wnd_params_init(&wparams);
	wparams.caption = params->caption;
	wparams.style |= ui_wds_maximize_btn | ui_wds_resizable;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 45;
		wparams.rect.p1.y = 21;

		wparams.min_size.x = 30;
		wparams.min_size.y = 18;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 340;
		wparams.rect.p1.y = 375;

		wparams.min_size.x = 240;
		wparams.min_size.y = 260;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &ui_file_dialog_wnd_cb, dialog);

	ui_window_get_app_rect(window, &arect);
	ui_res = ui_window_get_res(window);

	/* Compute geometry. */
	ui_file_dialog_get_geom(ui, &arect, &geom);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	/* File name label */

	rc = ui_label_create(ui_res, "File Name:", &label);
	if (rc != EOK)
		goto error;

	ui_label_set_rect(label, &geom.fname_label_rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	/* File name entry */

	rc = ui_entry_create(window, params->ifname, &entry);
	if (rc != EOK)
		goto error;

	ui_entry_set_rect(entry, &geom.fname_entry_rect);

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

	ui_label_set_rect(label, &geom.files_label_rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	/* File list */

	rc = ui_file_list_create(window, false, &flist);
	if (rc != EOK)
		goto error;

	ui_file_list_set_rect(flist, &geom.flist_rect);
	ui_file_list_set_cb(flist, &ui_file_dialog_flist_cb, dialog);

	rc = ui_fixed_add(fixed, ui_file_list_ctl(flist));
	if (rc != EOK)
		goto error;

	dialog->flist = flist;
	flist = NULL;

	rc = ui_file_list_read_dir(dialog->flist, ".");
	if (rc != EOK)
		goto error;

	/* Directory label */

	rc = ui_label_create(ui_res, "Directory:", &label);
	if (rc != EOK)
		goto error;

	ui_label_set_rect(label, &geom.dir_label_rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	dialog->ldir = label;
	label = NULL;

	/* Directory entry */

	rc = ui_entry_create(window, params->ifname, &entry);
	if (rc != EOK)
		goto error;

	ui_entry_set_rect(entry, &geom.dir_entry_rect);
	ui_entry_set_read_only(entry, true);

	rc = ui_fixed_add(fixed, ui_entry_ctl(entry));
	if (rc != EOK)
		goto error;

	dialog->edir = entry;
	entry = NULL;

	dirname = ui_file_list_get_dir(dialog->flist);
	if (dirname == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_entry_set_text(dialog->edir, dirname);
	if (rc != EOK)
		goto error;

	free(dirname);
	dirname = NULL;

	/* OK button */

	rc = ui_pbutton_create(ui_res, "OK", &bok);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bok, &ui_file_dialog_bok_cb, dialog);
	ui_pbutton_set_rect(bok, &geom.bok_rect);
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
	ui_pbutton_set_rect(bcancel, &geom.bcancel_rect);

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
	if (dirname != NULL)
		free(dirname);
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

/** File dialog window resize handler.
 *
 * @param window Window
 * @param arg Argument (ui_file_dialog_t *)
 */
static void ui_file_dialog_wnd_resize(ui_window_t *window, void *arg)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;
	gfx_rect_t arect;
	ui_file_dialog_geom_t geom;

	/* Get new window application rectangle. */
	ui_window_get_app_rect(window, &arect);

	/* Compute geometry. */
	ui_file_dialog_get_geom(ui_window_get_ui(window), &arect, &geom);

	ui_entry_set_rect(dialog->ename, &geom.fname_entry_rect);
	ui_file_list_set_rect(dialog->flist, &geom.flist_rect);
	ui_pbutton_set_rect(dialog->bok, &geom.bok_rect);
	ui_pbutton_set_rect(dialog->bcancel, &geom.bcancel_rect);
	ui_label_set_rect(dialog->ldir, &geom.dir_label_rect);
	ui_entry_set_rect(dialog->edir, &geom.dir_entry_rect);

	(void)ui_window_paint(window);
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
			fname = ui_entry_get_text(dialog->ename);
			ui_file_dialog_bok(dialog, fname);
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

	ui_file_dialog_bok(dialog, fname);
}

static void ui_file_dialog_flist_dir_selected(ui_file_list_t *flist, void *arg,
    const char *dname)
{
	ui_file_dialog_t *dialog = (ui_file_dialog_t *) arg;
	char *dirname;

	(void)ui_file_list_read_dir(flist, dname);

	dirname = ui_file_list_get_dir(flist);
	if (dirname != NULL) {
		(void)ui_entry_set_text(dialog->edir, dirname);
		free(dirname);
	}

	(void)ui_window_paint(dialog->window);
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

	fname = ui_entry_get_text(dialog->ename);
	ui_file_dialog_bok(dialog, fname);
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

/** Call file dialog bok callback to inform caller that dialog was confirmed.
 *
 * @param dialog File dialog
 * @param fname Selected file name
 */
static void ui_file_dialog_bok(ui_file_dialog_t *dialog, const char *fname)
{
	char *dfname;

	/*
	 * fname can point to an object that is part of dialog. The
	 * user handler might destroy dialog as part of its processing
	 * and later access the file name.
	 *
	 * Need pass it file name in a buffer that will remain valid
	 * until the completion of the user handler.
	 */
	dfname = str_dup(fname);
	if (dfname == NULL)
		return;

	if (dialog->cb != NULL && dialog->cb->bok != NULL)
		dialog->cb->bok(dialog, dialog->arg, dfname);

	free(dfname);
}

/** @}
 */
