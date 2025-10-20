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
/**
 * @file I/O Error Dialog
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
#include "ioerrdlg.h"

static void io_err_dlg_wnd_close(ui_window_t *, void *);
static void io_err_dlg_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t io_err_dlg_wnd_cb = {
	.close = io_err_dlg_wnd_close,
	.kbd = io_err_dlg_wnd_kbd
};

static void io_err_dlg_babort_clicked(ui_pbutton_t *, void *);
static void io_err_dlg_bretry_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t io_err_dlg_babort_cb = {
	.clicked = io_err_dlg_babort_clicked
};

ui_pbutton_cb_t io_err_dlg_bretry_cb = {
	.clicked = io_err_dlg_bretry_clicked
};

/** Initialize I/O error dialog parameters structure.
 *
 * I/O error parameters structure must always be initialized using
 * this function first.
 *
 * @param params I/O error dialog parameters structure
 */
void io_err_dlg_params_init(io_err_dlg_params_t *params)
{
	memset(params, 0, sizeof(io_err_dlg_params_t));
	params->text1 = "";
	params->text2 = "";
}

/** Create I/O error dialog.
 *
 * @param ui User interface
 * @param params Dialog parameters
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t io_err_dlg_create(ui_t *ui, io_err_dlg_params_t *params,
    io_err_dlg_t **rdialog)
{
	errno_t rc;
	io_err_dlg_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_pbutton_t *babort = NULL;
	ui_pbutton_t *bretry = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	char *name = NULL;

	dialog = calloc(1, sizeof(io_err_dlg_t));
	if (dialog == NULL) {
		rc = ENOMEM;
		goto error;
	}

	ui_wnd_params_init(&wparams);
	wparams.caption = "I/O Error";

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 60;
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

	ui_window_set_cb(window, &io_err_dlg_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	rc = ui_label_create(ui_res, params->text1, &label);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 2;
		rect.p1.x = 57;
		rect.p1.y = 3;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 190;
		rect.p1.y = 50;
	}

	ui_label_set_rect(label, &rect);
	ui_label_set_halign(label, gfx_halign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	rc = ui_label_create(ui_res, params->text2, &label);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 4;
		rect.p1.x = 57;
		rect.p1.y = 5;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 190;
		rect.p1.y = 50;
	}

	ui_label_set_rect(label, &rect);
	ui_label_set_halign(label, gfx_halign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	rc = ui_pbutton_create(ui_res, "Abort", &babort);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(babort, &io_err_dlg_babort_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 20;
		rect.p0.y = 6;
		rect.p1.x = 30;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 55;
		rect.p0.y = 120;
		rect.p1.x = 145;
		rect.p1.y = 148;
	}

	ui_pbutton_set_rect(babort, &rect);

	ui_pbutton_set_default(babort, true);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(babort));
	if (rc != EOK)
		goto error;

	dialog->babort = babort;
	babort = NULL;

	rc = ui_pbutton_create(ui_res, "Retry", &bretry);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bretry, &io_err_dlg_bretry_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 32;
		rect.p0.y = 6;
		rect.p1.x = 42;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 155;
		rect.p0.y = 120;
		rect.p1.x = 245;
		rect.p1.y = 148;
	}

	ui_pbutton_set_rect(bretry, &rect);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(bretry));
	if (rc != EOK)
		goto error;

	dialog->bretry = bretry;
	bretry = NULL;

	ui_window_add(window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(window);
	if (rc != EOK)
		goto error;

	dialog->window = window;
	*rdialog = dialog;
	return EOK;
error:
	if (name != NULL)
		free(name);
	if (babort != NULL)
		ui_pbutton_destroy(babort);
	if (bretry != NULL)
		ui_pbutton_destroy(bretry);
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

/** Destroy prompt dialog.
 *
 * @param dialog Prompt dialog or @c NULL
 */
void io_err_dlg_destroy(io_err_dlg_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set mesage dialog callback.
 *
 * @param dialog Prompt dialog
 * @param cb Prompt dialog callbacks
 * @param arg Callback argument
 */
void io_err_dlg_set_cb(io_err_dlg_t *dialog, io_err_dlg_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** Prompt dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (io_err_dlg_t *)
 */
static void io_err_dlg_wnd_close(ui_window_t *window, void *arg)
{
	io_err_dlg_t *dialog = (io_err_dlg_t *) arg;

	(void)window;
	if (dialog->cb != NULL && dialog->cb->close != NULL) {
		dialog->cb->close(dialog, dialog->arg);
	}
}

/** Prompt dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (io_err_dlg_t *)
 * @param event Keyboard event
 */
static void io_err_dlg_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	io_err_dlg_t *dialog = (io_err_dlg_t *) arg;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* Confirm */
			if (dialog->cb != NULL && dialog->cb->babort != NULL) {
				dialog->cb->babort(dialog, dialog->arg);
				return;
			}
		} else if (event->key == KC_ESCAPE) {
			/* Cancel */
			if (dialog->cb != NULL && dialog->cb->bretry != NULL) {
				dialog->cb->bretry(dialog, dialog->arg);
				return;
			}
		}
	}

	ui_window_def_kbd(window, event);
}

/** Prompt dialog OK button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (io_err_dlg_t *)
 */
static void io_err_dlg_babort_clicked(ui_pbutton_t *pbutton, void *arg)
{
	io_err_dlg_t *dialog = (io_err_dlg_t *) arg;

	if (dialog->cb != NULL && dialog->cb->babort != NULL) {
		dialog->cb->babort(dialog, dialog->arg);
	}
}

/** Prompt dialog cancel button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (io_err_dlg_t *)
 */
static void io_err_dlg_bretry_clicked(ui_pbutton_t *pbutton, void *arg)
{
	io_err_dlg_t *dialog = (io_err_dlg_t *) arg;

	(void)pbutton;
	if (dialog->cb != NULL && dialog->cb->bretry != NULL) {
		dialog->cb->bretry(dialog, dialog->arg);
	}
}

/** @}
 */
