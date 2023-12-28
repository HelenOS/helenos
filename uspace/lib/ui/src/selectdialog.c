/*
 * Copyright (c) 2023 Jiri Svoboda
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
 * @file Select dialog
 */

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/selectdialog.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/selectdialog.h"

static void ui_select_dialog_wnd_close(ui_window_t *, void *);
static void ui_select_dialog_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t ui_select_dialog_wnd_cb = {
	.close = ui_select_dialog_wnd_close,
	.kbd = ui_select_dialog_wnd_kbd
};

static void ui_select_dialog_bok_clicked(ui_pbutton_t *, void *);
static void ui_select_dialog_bcancel_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t ui_select_dialog_bok_cb = {
	.clicked = ui_select_dialog_bok_clicked
};

ui_pbutton_cb_t ui_select_dialog_bcancel_cb = {
	.clicked = ui_select_dialog_bcancel_clicked
};

static void ui_select_dialog_list_selected(ui_list_entry_t *, void *);

ui_list_cb_t ui_select_dialog_list_cb = {
	.selected = ui_select_dialog_list_selected
};

/** Initialize select dialog parameters structure.
 *
 * Select dialog parameters structure must always be initialized using
 * this function first.
 *
 * @param params Select dialog parameters structure
 */
void ui_select_dialog_params_init(ui_select_dialog_params_t *params)
{
	memset(params, 0, sizeof(ui_select_dialog_params_t));
}

/** Create new select dialog.
 *
 * @param ui User interface
 * @param params Select dialog parameters
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t ui_select_dialog_create(ui_t *ui, ui_select_dialog_params_t *params,
    ui_select_dialog_t **rdialog)
{
	errno_t rc;
	ui_select_dialog_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_list_t *list = NULL;
	ui_pbutton_t *bok = NULL;
	ui_pbutton_t *bcancel = NULL;
	gfx_rect_t rect;
	ui_resource_t *ui_res;

	dialog = calloc(1, sizeof(ui_select_dialog_t));
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
		wparams.rect.p1.x = 55;
		wparams.rect.p1.y = 19;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 450;
		wparams.rect.p1.y = 235;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &ui_select_dialog_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	rc = ui_label_create(ui_res, params->prompt, &label);
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

	rc = ui_list_create(window, true, &list);
	if (rc != EOK)
		goto error;

	ui_list_set_cb(list, &ui_select_dialog_list_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 4;
		rect.p1.x = 52;
		rect.p1.y = 15;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 55;
		rect.p1.x = 440;
		rect.p1.y = 180;
	}

	ui_list_set_rect(list, &rect);

	rc = ui_fixed_add(fixed, ui_list_ctl(list));
	if (rc != EOK)
		goto error;

	dialog->list = list;
	list = NULL;

	rc = ui_pbutton_create(ui_res, "OK", &bok);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(bok, &ui_select_dialog_bok_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 16;
		rect.p0.y = 16;
		rect.p1.x = 26;
		rect.p1.y = 17;
	} else {
		rect.p0.x = 130;
		rect.p0.y = 190;
		rect.p1.x = 220;
		rect.p1.y = 218;
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

	ui_pbutton_set_cb(bcancel, &ui_select_dialog_bcancel_cb, dialog);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 28;
		rect.p0.y = 16;
		rect.p1.x = 38;
		rect.p1.y = 17;
	} else {
		rect.p0.x = 230;
		rect.p0.y = 190;
		rect.p1.x = 320;
		rect.p1.y = 218;
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
	if (list != NULL)
		ui_list_destroy(list);
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

/** Destroy select dialog.
 *
 * @param dialog Select dialog or @c NULL
 */
void ui_select_dialog_destroy(ui_select_dialog_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set mesage dialog callback.
 *
 * @param dialog Select dialog
 * @param cb Select dialog callbacks
 * @param arg Callback argument
 */
void ui_select_dialog_set_cb(ui_select_dialog_t *dialog, ui_select_dialog_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** Append new entry to select dialog.
 *
 * @param dialog Select dialog
 * @param attr List entry attributes
 * @return EOK on success or an error code
 */
errno_t ui_select_dialog_append(ui_select_dialog_t *dialog,
    ui_list_entry_attr_t *attr)
{
	return ui_list_entry_append(dialog->list, attr, NULL);
}

/** Paint select dialog.
 *
 * This needs to be called after appending entries.
 *
 * @param dialog Select dialog
 * @return EOK on success or an error code
 */
errno_t ui_select_dialog_paint(ui_select_dialog_t *dialog)
{
	return ui_window_paint(dialog->window);
}

/** Get the entry list from select dialog.
 *
 * @param dialog Select dialog
 * @return UI list
 */
ui_list_t *ui_select_dialog_list(ui_select_dialog_t *dialog)
{
	return dialog->list;
}

/** Select dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (ui_select_dialog_t *)
 */
static void ui_select_dialog_wnd_close(ui_window_t *window, void *arg)
{
	ui_select_dialog_t *dialog = (ui_select_dialog_t *) arg;

	if (dialog->cb != NULL && dialog->cb->close != NULL)
		dialog->cb->close(dialog, dialog->arg);
}

/** Select dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (ui_select_dialog_t *)
 * @param event Keyboard event
 */
static void ui_select_dialog_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	ui_select_dialog_t *dialog = (ui_select_dialog_t *) arg;
	ui_list_entry_t *entry;
	void *earg;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* Confirm */
			if (dialog->cb != NULL && dialog->cb->bok != NULL) {
				entry = ui_list_get_cursor(dialog->list);
				earg = ui_list_entry_get_arg(entry);
				dialog->cb->bok(dialog, dialog->arg, earg);
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

/** Select dialog OK button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_select_dialog_t *)
 */
static void ui_select_dialog_bok_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_select_dialog_t *dialog = (ui_select_dialog_t *) arg;
	ui_list_entry_t *entry;
	void *earg;

	if (dialog->cb != NULL && dialog->cb->bok != NULL) {
		entry = ui_list_get_cursor(dialog->list);
		if (entry != NULL)
			earg = ui_list_entry_get_arg(entry);
		else
			earg = NULL;

		dialog->cb->bok(dialog, dialog->arg, earg);
	}
}

/** Select dialog cancel button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_select_dialog_t *)
 */
static void ui_select_dialog_bcancel_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_select_dialog_t *dialog = (ui_select_dialog_t *) arg;

	if (dialog->cb != NULL && dialog->cb->bcancel != NULL)
		dialog->cb->bcancel(dialog, dialog->arg);
}

/** Select dialog list entry selection handler.
 *
 * @param entry UI list entry
 * @param arg Entry argument
 */
static void ui_select_dialog_list_selected(ui_list_entry_t *entry, void *arg)
{
	ui_list_t *list;
	ui_select_dialog_t *dialog;

	list = ui_list_entry_get_list(entry);
	dialog = (ui_select_dialog_t *)ui_list_get_cb_arg(list);

	if (dialog->cb != NULL && dialog->cb->bok != NULL)
		dialog->cb->bok(dialog, dialog->arg, arg);
}

/** @}
 */
