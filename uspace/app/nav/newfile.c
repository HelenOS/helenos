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
 * @file Navigator New File.
 */

#include <stdlib.h>
#include <str_error.h>
#include <ui/fixed.h>
#include <ui/filelist.h>
#include <ui/msgdialog.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "dlg/newfiledlg.h"
#include "menu.h"
#include "newfile.h"
#include "nav.h"

static void new_file_bok(new_file_dlg_t *, void *, const char *);
static void new_file_bcancel(new_file_dlg_t *, void *);
static void new_file_close(new_file_dlg_t *, void *);

static new_file_dlg_cb_t new_file_cb = {
	.bok = new_file_bok,
	.bcancel = new_file_bcancel,
	.close = new_file_close
};

/** Open New File dialog.
 *
 * @param navigator Navigator
 */
void navigator_new_file_dlg(navigator_t *navigator)
{
	new_file_dlg_t *dlg;

	new_file_dlg_create(navigator->ui, &dlg);
	new_file_dlg_set_cb(dlg, &new_file_cb, (void *)navigator);
}

/** New file dialog confirmed.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 * @param fname New file name
 */
static void new_file_bok(new_file_dlg_t *dlg, void *arg, const char *fname)
{
	navigator_t *nav = (navigator_t *)arg;
	ui_msg_dialog_t *dialog = NULL;
	ui_msg_dialog_params_t params;
	char *msg = NULL;
	int rv;
	FILE *f;

	new_file_dlg_destroy(dlg);
	f = fopen(fname, "wx");
	if (f == NULL) {
		rv = asprintf(&msg, "Error creating file (%s).",
		    str_error(errno));
		if (rv < 0)
			return;

		ui_msg_dialog_params_init(&params);
		params.caption = "Error";
		params.text = msg;
		(void) ui_msg_dialog_create(nav->ui, &params, &dialog);
		free(msg);
		return;
	}

	fclose(f);
	navigator_refresh_panels(nav);
}

/** New file dialog cancelled.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 */
static void new_file_bcancel(new_file_dlg_t *dlg, void *arg)
{
	(void)arg;
	new_file_dlg_destroy(dlg);
}

/** New file dialog closed.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 */
static void new_file_close(new_file_dlg_t *dlg, void *arg)
{
	(void)arg;
	new_file_dlg_destroy(dlg);
}

/** @}
 */
