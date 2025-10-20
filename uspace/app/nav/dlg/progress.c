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
 * @file Progress dialog
 */

#include <errno.h>
#include <fmgt.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "progress.h"

static void progress_dlg_wnd_close(ui_window_t *, void *);
static void progress_dlg_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t progress_dlg_wnd_cb = {
	.close = progress_dlg_wnd_close,
	.kbd = progress_dlg_wnd_kbd
};

static void progress_dlg_babort_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t progress_dlg_babort_cb = {
	.clicked = progress_dlg_babort_clicked
};

/** Initialize progress dialog parameters structure.
 *
 * Progress dialog parameters structure must always be initialized using
 * this function first.
 *
 * @param params Progress dialog parameters structure
 */
void progress_dlg_params_init(progress_dlg_params_t *params)
{
	memset(params, 0, sizeof(progress_dlg_params_t));
	params->caption = "";
}

/** Create progress dialog.
 *
 * @param ui User interface
 * @param params Parameters
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t progress_dlg_create(ui_t *ui, progress_dlg_params_t *params,
    progress_dlg_t **rdialog)
{
	errno_t rc;
	progress_dlg_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_pbutton_t *babort = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	char *name = NULL;

	dialog = calloc(1, sizeof(progress_dlg_t));
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
		wparams.rect.p1.x = 50;
		wparams.rect.p1.y = 11;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 400;
		wparams.rect.p1.y = 135;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &progress_dlg_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	rc = ui_label_create(ui_res, "XXX of XXX (XXX%)", &label);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 2;
		rect.p1.x = 47;
		rect.p1.y = 3;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 390;
		rect.p1.y = 50;
	}

	ui_label_set_rect(label, &rect);
	ui_label_set_halign(label, gfx_halign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	dialog->lcurf_prog = label;
	label = NULL;

	rc = ui_pbutton_create(ui_res, "Abort", &babort);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(babort, &progress_dlg_babort_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 20;
		rect.p0.y = 8;
		rect.p1.x = 30;
		rect.p1.y = 9;
	} else {
		rect.p0.x = 205;
		rect.p0.y = 90;
		rect.p1.x = 295;
		rect.p1.y = 118;
	}

	ui_pbutton_set_rect(babort, &rect);

	ui_pbutton_set_default(babort, true);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(babort));
	if (rc != EOK)
		goto error;

	dialog->babort = babort;
	babort = NULL;

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

/** Destroy progress dialog.
 *
 * @param dialog Progress dialog or @c NULL
 */
void progress_dlg_destroy(progress_dlg_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set mesage dialog callback.
 *
 * @param dialog Progress dialog
 * @param cb Progress dialog callbacks
 * @param arg Callback argument
 */
void progress_dlg_set_cb(progress_dlg_t *dialog, progress_dlg_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** Set current file progress text.
 *
 * @param dialog Progress dialog
 * @param text New text for current file progress
 *
 * @return EOK on success or an error code
 */
errno_t progress_dlg_set_curf_prog(progress_dlg_t *dialog, const char *text)
{
	errno_t rc;

	rc = ui_label_set_text(dialog->lcurf_prog, text);
	if (rc != EOK)
		return rc;

	return ui_label_paint(dialog->lcurf_prog);
}

/** Progress dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (progress_dlg_t *)
 */
static void progress_dlg_wnd_close(ui_window_t *window, void *arg)
{
	progress_dlg_t *dialog = (progress_dlg_t *) arg;

	(void)window;
	if (dialog->cb != NULL && dialog->cb->close != NULL) {
		dialog->cb->close(dialog, dialog->arg);
	}
}

/** Progress dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (progress_dlg_t *)
 * @param event Keyboard event
 */
static void progress_dlg_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	progress_dlg_t *dialog = (progress_dlg_t *) arg;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER || event->key == KC_ESCAPE) {
			/* Abort */
			if (dialog->cb != NULL && dialog->cb->babort != NULL) {
				dialog->cb->babort(dialog, dialog->arg);
				return;
			}
		}
	}

	ui_window_def_kbd(window, event);
}

/** Progress dialog Abort button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (progress_dlg_t *)
 */
static void progress_dlg_babort_clicked(ui_pbutton_t *pbutton, void *arg)
{
	progress_dlg_t *dialog = (progress_dlg_t *) arg;

	if (dialog->cb != NULL && dialog->cb->babort != NULL) {
		dialog->cb->babort(dialog, dialog->arg);
	}
}

/** @}
 */
