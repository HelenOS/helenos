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
 * @file Message dialog
 */

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/msgdialog.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "../private/msgdialog.h"

static void ui_msg_dialog_wnd_close(ui_window_t *, void *);
static void ui_msg_dialog_wnd_kbd(ui_window_t *, void *, kbd_event_t *);

ui_window_cb_t ui_msg_dialog_wnd_cb = {
	.close = ui_msg_dialog_wnd_close,
	.kbd = ui_msg_dialog_wnd_kbd
};

static void ui_msg_dialog_btn_clicked(ui_pbutton_t *, void *);

ui_pbutton_cb_t ui_msg_dialog_btn_cb = {
	.clicked = ui_msg_dialog_btn_clicked
};

static const char *ui_msg_dialog_captions[][ui_msg_dialog_maxbtn + 1] = {
	[umdc_ok] = { "OK", NULL },
	[umdc_ok_cancel] = { "OK", "Cancel", NULL }
};

/** Initialize message dialog parameters structure.
 *
 * Message dialog parameters structure must always be initialized using
 * this function first.
 *
 * @param params Message dialog parameters structure
 */
void ui_msg_dialog_params_init(ui_msg_dialog_params_t *params)
{
	memset(params, 0, sizeof(ui_msg_dialog_params_t));
}

/** Create new message dialog.
 *
 * @param ui User interface
 * @param params Message dialog parameters
 * @param rdialog Place to store pointer to new dialog
 * @return EOK on success or an error code
 */
errno_t ui_msg_dialog_create(ui_t *ui, ui_msg_dialog_params_t *params,
    ui_msg_dialog_t **rdialog)
{
	errno_t rc;
	ui_msg_dialog_t *dialog;
	ui_window_t *window = NULL;
	ui_wnd_params_t wparams;
	ui_fixed_t *fixed = NULL;
	ui_label_t *label = NULL;
	ui_pbutton_t *btn[ui_msg_dialog_maxbtn];
	const char **cp;
	unsigned i, nb;
	gfx_rect_t rect;
	gfx_coord_t bw, bpad, btnsw, bp0x;
	ui_resource_t *ui_res;

	for (i = 0; i < ui_msg_dialog_maxbtn; i++)
		btn[i] = NULL;

	dialog = calloc(1, sizeof(ui_msg_dialog_t));
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
		wparams.rect.p1.x = 60;
		wparams.rect.p1.y = 7;
	} else {
		wparams.rect.p0.x = 0;
		wparams.rect.p0.y = 0;
		wparams.rect.p1.x = 400;
		wparams.rect.p1.y = 110;
	}

	rc = ui_window_create(ui, &wparams, &window);
	if (rc != EOK)
		goto error;

	ui_window_set_cb(window, &ui_msg_dialog_wnd_cb, dialog);

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK)
		goto error;

	rc = ui_label_create(ui_res, params->text, &label);
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
		rect.p1.x = 390;
		rect.p1.y = 50;
	}

	ui_label_set_rect(label, &rect);
	ui_label_set_halign(label, gfx_halign_center);

	rc = ui_fixed_add(fixed, ui_label_ctl(label));
	if (rc != EOK)
		goto error;

	label = NULL;

	i = 0;
	assert(params->choice == umdc_ok || params->choice == umdc_ok_cancel);
	cp = ui_msg_dialog_captions[params->choice];

	while (*cp != NULL) {
		rc = ui_pbutton_create(ui_res, *cp, &btn[i]);
		if (rc != EOK)
			goto error;

		ui_pbutton_set_cb(btn[i], &ui_msg_dialog_btn_cb, dialog);
		++cp;
		++i;
	}

	nb = i;

	if (ui_is_textmode(ui)) {
		bw = 12;
		bpad = 2;
	} else {
		bw = 90;
		bpad = 10;
	}

	btnsw = (nb - 1) * (bw + bpad) + bw;
	bp0x = (wparams.rect.p0.x + wparams.rect.p1.x - btnsw) / 2;

	for (i = 0; i < nb; i++) {
		/* FIXME: Auto layout */
		if (ui_is_textmode(ui)) {
			rect.p0.x = bp0x + i * (bw + bpad);
			rect.p0.y = 4;
			rect.p1.x = bp0x + bw + i * (bw + bpad);
			rect.p1.y = 5;
		} else {
			rect.p0.x = bp0x + i * (bw + bpad);
			rect.p0.y = 60;
			rect.p1.x = bp0x + bw + i * (bw + bpad);
			rect.p1.y = 88;
		}

		ui_pbutton_set_rect(btn[i], &rect);

		rc = ui_fixed_add(fixed, ui_pbutton_ctl(btn[i]));
		if (rc != EOK)
			goto error;
	}

	ui_pbutton_set_default(btn[0], true);

	for (i = 0; i < ui_msg_dialog_maxbtn; i++) {
		dialog->btn[i] = btn[i];
		btn[i] = NULL;
	}

	ui_window_add(window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(window);
	if (rc != EOK)
		goto error;

	dialog->window = window;
	*rdialog = dialog;
	return EOK;
error:
	for (i = 0; i < ui_msg_dialog_maxbtn; i++) {
		if (btn[i] != NULL)
			ui_pbutton_destroy(btn[i]);
	}
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

/** Destroy message dialog.
 *
 * @param dialog Message dialog or @c NULL
 */
void ui_msg_dialog_destroy(ui_msg_dialog_t *dialog)
{
	if (dialog == NULL)
		return;

	ui_window_destroy(dialog->window);
	free(dialog);
}

/** Set mesage dialog callback.
 *
 * @param dialog Message dialog
 * @param cb Message dialog callbacks
 * @param arg Callback argument
 */
void ui_msg_dialog_set_cb(ui_msg_dialog_t *dialog, ui_msg_dialog_cb_t *cb,
    void *arg)
{
	dialog->cb = cb;
	dialog->arg = arg;
}

/** Message dialog window close handler.
 *
 * @param window Window
 * @param arg Argument (ui_msg_dialog_t *)
 */
static void ui_msg_dialog_wnd_close(ui_window_t *window, void *arg)
{
	ui_msg_dialog_t *dialog = (ui_msg_dialog_t *) arg;

	if (dialog->cb != NULL && dialog->cb->close != NULL)
		dialog->cb->close(dialog, dialog->arg);
}

/** Message dialog window keyboard event handler.
 *
 * @param window Window
 * @param arg Argument (ui_msg_dialog_t *)
 * @param event Keyboard event
 */
static void ui_msg_dialog_wnd_kbd(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	ui_msg_dialog_t *dialog = (ui_msg_dialog_t *) arg;
	ui_evclaim_t claim;

	claim = ui_window_def_kbd(window, event);
	if (claim == ui_claimed)
		return;

	if (event->type == KEY_PRESS &&
	    (event->mods & (KM_CTRL | KM_SHIFT | KM_ALT)) == 0) {
		if (event->key == KC_ENTER) {
			/* OK / default button */
			if (dialog->cb != NULL && dialog->cb->button != NULL) {
				dialog->cb->button(dialog, dialog->arg, 0);
				return;
			}
		} else if (event->key == KC_ESCAPE) {
			/* Cancel */
			if (dialog->cb != NULL && dialog->cb->close != NULL) {
				dialog->cb->close(dialog, dialog->arg);
				return;
			}
		}
	}

}

/** Message dialog button click handler.
 *
 * @param pbutton Push button
 * @param arg Argument (ui_msg_dialog_t *)
 */
static void ui_msg_dialog_btn_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_msg_dialog_t *dialog = (ui_msg_dialog_t *) arg;
	unsigned i;

	if (dialog->cb != NULL && dialog->cb->button != NULL) {
		for (i = 0; i < ui_msg_dialog_maxbtn; i++) {
			if (dialog->btn[i] == pbutton)
				dialog->cb->button(dialog, dialog->arg, i);
		}
	}
}

/** @}
 */
