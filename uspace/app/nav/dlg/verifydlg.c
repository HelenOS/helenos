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

/** @addtogroup nav
 * @{
 */
/**
 * @file Verify dialog
 */

#include <errno.h>
#include <fmgt.h>
#include <mem.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "verifydlg.h"

static void verify_dlg_wnd_close(ui_window_t *, void *);
static void verify_dlg_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t verify_dlg_wnd_cb = {
	.close = verify_dlg_wnd_close,
	.kbd = verify_dlg_wnd_kbd
};

static void verify_dlg_bok_clicked(ui_pbutton_t *, void *);
static void verify_dlg_bcancel_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t verify_dlg_bok_cb = {
	.clicked = verify_dlg_bok_clicked
};

ui_pbutton_cb_t verify_dlg_bcancel_cb = {
	.clicked = verify_dlg_bcancel_clicked
};

/** Create Verify dialog.
 *
 * @param ui User interface
 * @param flist List of files to verify (ownership transferred)
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t verify_dlg_create(ui_t *ui, fmgt_flist_t *flist,
    verify_dlg_t **rdialog)
{
	errno_t rc;
	verify_dlg_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_pbutton_t *bok = NULL;
	ui_pbutton_t *bcancel = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	fmgt_flist_entry_t *entry;
	char *tverify = NULL;
	unsigned count;

	dialog = calloc(1, sizeof(verify_dlg_t));
	if (dialog == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ui_wnd_params_init(&wparams);
	wparams.caption = "Verify";

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 40;
		wparams.rect.p1.y = 9;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 300;
		wparams.rect.p1.y = 155;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &verify_dlg_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	count = fmgt_flist_count(flist);
	if (count == 1) {
		entry = fmgt_flist_first(flist);
		rc = asprintf(&tverify, "Verify \"%s\":",
		    entry->fname);
	} else {
		rc = asprintf(&tverify, "Verify %u files/directories:",
		    count);
	}

	rc = ui_label_create(ui_res, tverify, &label);
	if (rc != EOK)
		goto error;

	free(tverify);
	tverify = NULL;

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

	rc = ui_pbutton_create(ui_res, "OK", &bok);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bok, &verify_dlg_bok_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 10;
		rect.p0.y = 6;
		rect.p1.x = 20;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 55;
		rect.p0.y = 120;
		rect.p1.x = 145;
		rect.p1.y = 148;
	}

	ui_pbutton_set_rect(bok, &rect);

	ui_pbutton_set_default(bok, true);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(bok));
	if (rc != EOK)
		goto error;

	dialog->bok = bok;
	bok = NULL;

	rc = ui_pbutton_create(ui_res, "Cancel", &bcancel);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bcancel, &verify_dlg_bcancel_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 22;
		rect.p0.y = 6;
		rect.p1.x = 32;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 155;
		rect.p0.y = 120;
		rect.p1.x = 245;
		rect.p1.y = 148;
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
	dialog->flist = flist;
	*rdialog = dialog;
	return EOK;
error:
	if (tverify != NULL)
		free(tverify);
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

/** Destroy verify dialog.
 *
 * @param dialog Verify dialog or @c NULL
 */
void verify_dlg_destroy(verify_dlg_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set verify dialog callback.
 *
 * @param dialog Verify dialog
 * @param cb Verify dialog callbacks
 * @param arg Callback argument
 */
void verify_dlg_set_cb(verify_dlg_t *dialog, verify_dlg_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** Verify dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (verify_dlg_t *)
 */
static void verify_dlg_wnd_close(ui_window_t *window, void *arg)
{
	verify_dlg_t *dialog = (verify_dlg_t *) arg;

	(void)window;
	if (dialog->cb != NULL && dialog->cb->close != NULL) {
		dialog->cb->close(dialog, dialog->arg);
	}
}

/** Verify dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (verify_dlg_t *)
 * @param event Keyboard event
 */
static void verify_dlg_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	verify_dlg_t *dialog = (verify_dlg_t *) arg;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* Confirm */
			if (dialog->cb != NULL && dialog->cb->bok != NULL) {
				dialog->cb->bok(dialog, dialog->arg);
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

	ui_window_def_kbd(window, event);
}

/** Verify dialog OK button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (verify_dlg_t *)
 */
static void verify_dlg_bok_clicked(ui_pbutton_t *pbutton, void *arg)
{
	verify_dlg_t *dialog = (verify_dlg_t *) arg;

	if (dialog->cb != NULL && dialog->cb->bok != NULL) {
		dialog->cb->bok(dialog, dialog->arg);
	}
}

/** Verify dialog cancel button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (verify_dlg_t *)
 */
static void verify_dlg_bcancel_clicked(ui_pbutton_t *pbutton, void *arg)
{
	verify_dlg_t *dialog = (verify_dlg_t *) arg;

	(void)pbutton;
	if (dialog->cb != NULL && dialog->cb->bcancel != NULL) {
		dialog->cb->bcancel(dialog, dialog->arg);
	}
}

/** @}
 */
