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

/** @addtogroup taskbar-cfg
 * @{
 */
/** @file Start menu entry edit dialog
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <stdlib.h>
#include <ui/checkbox.h>
#include <ui/fixed.h>
#include <ui/resource.h>
#include <ui/window.h>
#include "taskbar-cfg.h"
#include "smeedit.h"
#include "startmenu.h"

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

static void smeedit_ok_clicked(ui_pbutton_t *, void *);
static void smeedit_cancel_clicked(ui_pbutton_t *, void *);

/** OK button callbacks */
ui_pbutton_cb_t smeedit_ok_button_cb = {
	.clicked = smeedit_ok_clicked
};

/** Cancel button callbacks */
ui_pbutton_cb_t smeedit_cancel_button_cb = {
	.clicked = smeedit_cancel_clicked
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (tbcfg)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	smeedit_t *smee = (smeedit_t *)arg;

	(void)window;
	smeedit_destroy(smee);
}

/** Create start menu entry edit dialog.
 *
 * @param smenu Start menu
 * @param smentry Start menu entry to edit or @c NULL if creating
 *                a new entry
 * @param rsmee Place to store pointer to new start menu entry edit window
 * @return EOK on success or an error code
 */
errno_t smeedit_create(startmenu_t *smenu, startmenu_entry_t *smentry,
    smeedit_t **rsmee)
{
	ui_wnd_params_t params;
	ui_t *ui;
	ui_window_t *window = NULL;
	smeedit_t *smee = NULL;
	gfx_rect_t rect;
	ui_resource_t *res;
	const char *cmd;
	const char *caption;
	bool terminal;
	errno_t rc;

	ui = smenu->tbarcfg->ui;

	if (smentry != NULL) {
		cmd = smenu_entry_get_cmd(smentry->entry);
		caption = smenu_entry_get_caption(smentry->entry);
		terminal = smenu_entry_get_terminal(smentry->entry);
	} else {
		cmd = "";
		caption = "";
		terminal = false;
	}

	smee = calloc(1, sizeof(smeedit_t));
	if (smee == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	smee->startmenu = smenu;
	smee->smentry = smentry;

	ui_wnd_params_init(&params);
	params.caption = smentry != NULL ? "Edit Start Menu Entry" :
	    "Create Start Menu Entry";
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 50;
		params.rect.p1.y = 13;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 370;
		params.rect.p1.y = 230;
	}

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(window, &window_cb, (void *)smee);
	smee->window = window;

	res = ui_window_get_res(window);

	rc = ui_fixed_create(&smee->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	/* Command to run label */

	rc = ui_label_create(res, "Command to run:", &smee->lcmd);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 2;
		rect.p1.x = 48;
		rect.p1.y = 3;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 35;
		rect.p1.x = 190;
		rect.p1.y = 50;
	}

	ui_label_set_rect(smee->lcmd, &rect);

	rc = ui_fixed_add(smee->fixed, ui_label_ctl(smee->lcmd));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Command entry */

	rc = ui_entry_create(window, cmd, &smee->ecmd);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 3;
		rect.p1.x = 48;
		rect.p1.y = 4;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 50;
		rect.p1.x = 360;
		rect.p1.y = 75;
	}

	ui_entry_set_rect(smee->ecmd, &rect);

	rc = ui_fixed_add(smee->fixed, ui_entry_ctl(smee->ecmd));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Caption label */

	rc = ui_label_create(res, "Caption:", &smee->lcaption);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 5;
		rect.p1.x = 20;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 95;
		rect.p1.x = 190;
		rect.p1.y = 110;
	}

	ui_label_set_rect(smee->lcaption, &rect);

	rc = ui_fixed_add(smee->fixed, ui_label_ctl(smee->lcaption));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Caption entry */

	rc = ui_entry_create(window, caption, &smee->ecaption);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 6;
		rect.p1.x = 48;
		rect.p1.y = 7;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 110;
		rect.p1.x = 360;
		rect.p1.y = 135;
	}

	ui_entry_set_rect(smee->ecaption, &rect);

	rc = ui_fixed_add(smee->fixed, ui_entry_ctl(smee->ecaption));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Start in terminal checkbox */

	rc = ui_checkbox_create(res, "Start in terminal", &smee->cbterminal);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 3;
		rect.p0.y = 8;
		rect.p1.x = 6;
		rect.p1.y = 9;
	} else {
		rect.p0.x = 10;
		rect.p0.y = 155;
		rect.p1.x = 360;
		rect.p1.y = 170;
	}

	ui_checkbox_set_rect(smee->cbterminal, &rect);
	ui_checkbox_set_checked(smee->cbterminal, terminal);

	rc = ui_fixed_add(smee->fixed, ui_checkbox_ctl(smee->cbterminal));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* OK button */

	rc = ui_pbutton_create(res, "OK", &smee->bok);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 23;
		rect.p0.y = 10;
		rect.p1.x = 35;
		rect.p1.y = 11;
	} else {
		rect.p0.x = 190;
		rect.p0.y = 190;
		rect.p1.x = 270;
		rect.p1.y = 215;
	}

	ui_pbutton_set_cb(smee->bok, &smeedit_ok_button_cb, (void *)smee);
	ui_pbutton_set_rect(smee->bok, &rect);
	ui_pbutton_set_default(smee->bok, true);

	rc = ui_fixed_add(smee->fixed, ui_pbutton_ctl(smee->bok));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* Cancel button */

	rc = ui_pbutton_create(res, "Cancel", &smee->bcancel);
	if (rc != EOK)
		goto error;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 36;
		rect.p0.y = 10;
		rect.p1.x = 48;
		rect.p1.y = 11;
	} else {
		rect.p0.x = 280;
		rect.p0.y = 190;
		rect.p1.x = 360;
		rect.p1.y = 215;
	}

	ui_pbutton_set_cb(smee->bcancel, &smeedit_cancel_button_cb,
	    (void *)smee);
	ui_pbutton_set_rect(smee->bcancel, &rect);

	rc = ui_fixed_add(smee->fixed, ui_pbutton_ctl(smee->bcancel));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_window_add(window, ui_fixed_ctl(smee->fixed));

	rc = ui_window_paint(window);
	if (rc != EOK)
		goto error;

	*rsmee = smee;
	return EOK;
error:
	if (smee->window != NULL)
		ui_window_destroy(smee->window);
	free(smee);
	return rc;
}

/** Destroy Taskbar configuration window.
 *
 * @param smee Start menu entry edit dialog
 */
void smeedit_destroy(smeedit_t *smee)
{
	ui_window_destroy(smee->window);
}

/** OK button clicked.
 *
 * @params bok OK button
 * @params arg Argument (smeedit_t *)
 */
static void smeedit_ok_clicked(ui_pbutton_t *bok, void *arg)
{
	smeedit_t *smee;
	smenu_entry_t *entry;
	startmenu_entry_t *smentry;
	const char *cmd;
	const char *caption;
	bool terminal;
	errno_t rc;

	(void)bok;
	smee = (smeedit_t *)arg;

	cmd = ui_entry_get_text(smee->ecmd);
	caption = ui_entry_get_text(smee->ecaption);
	terminal = ui_checkbox_get_checked(smee->cbterminal);

	if (smee->smentry == NULL) {
		/* Create new entry */
		rc = smenu_entry_create(smee->startmenu->tbarcfg->tbarcfg,
		    caption, cmd, terminal, &entry);
		if (rc != EOK)
			return;

		rc = startmenu_insert(smee->startmenu, entry, &smentry);
		if (rc != EOK)
			return;

		startmenu_repaint(smee->startmenu);
		(void)tbarcfg_sync(smee->startmenu->tbarcfg->tbarcfg);
		(void)tbarcfg_notify(TBARCFG_NOTIFY_DEFAULT);
	} else {
		/* Edit existing entry */
		rc = smenu_entry_set_cmd(smee->smentry->entry, cmd);
		if (rc != EOK)
			return;

		smenu_entry_set_caption(smee->smentry->entry, caption);
		if (rc != EOK)
			return;

		smenu_entry_set_terminal(smee->smentry->entry, terminal);
		if (rc != EOK)
			return;

		(void)tbarcfg_sync(smee->startmenu->tbarcfg->tbarcfg);
		startmenu_entry_update(smee->smentry);
		(void)tbarcfg_sync(smee->startmenu->tbarcfg->tbarcfg);
		(void)tbarcfg_notify(TBARCFG_NOTIFY_DEFAULT);
	}

	smeedit_destroy(smee);
}

/** Cancel button clicked.
 *
 * @params bok OK button
 * @params arg Argument (smeedit_t *)
 */
static void smeedit_cancel_clicked(ui_pbutton_t *bcancel, void *arg)
{
	smeedit_t *smee;

	(void)bcancel;
	smee = (smeedit_t *)arg;
	smeedit_destroy(smee);
}

/** @}
 */
