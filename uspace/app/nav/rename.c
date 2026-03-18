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
 * @file Navigator Rename file or directory.
 */

#include <fmgt.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <ui/fixed.h>
#include <ui/filelist.h>
#include <ui/msgdialog.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <stdbool.h>
#include <str.h>
#include "dlg/renamedlg.h"
#include "dlg/progress.h"
#include "menu.h"
#include "nav.h"
#include "rename.h"
#include "types/rename.h"

static void rename_bok(rename_dlg_t *, void *, const char *);
static void rename_bcancel(rename_dlg_t *, void *);
static void rename_close(rename_dlg_t *, void *);

static rename_dlg_cb_t rename_cb = {
	.bok = rename_bok,
	.bcancel = rename_bcancel,
	.close = rename_close
};

static bool rename_abort_query(void *);
static void rename_progress(void *, fmgt_progress_t *);

static fmgt_cb_t rename_fmgt_cb = {
	.abort_query = rename_abort_query,
	.io_error_query = navigator_io_error_query,
	.progress = rename_progress,
};

/** Open New Directory dialog.
 *
 * @param navigator Navigator
 * @param old_path Old path
 */
void navigator_rename_dlg(navigator_t *navigator, const char *old_path)
{
	rename_dlg_t *dlg;

	rename_dlg_create(navigator->ui, old_path, &dlg);
	rename_dlg_set_cb(dlg, &rename_cb, (void *)navigator);
}

/** Rename worker function.
 *
 * @param arg Argument (navigator_rename_job_t)
 */
static void rename_wfunc(void *arg)
{
	fmgt_t *fmgt = NULL;
	navigator_rename_job_t *job = (navigator_rename_job_t *)arg;
	char *msg = NULL;
	navigator_t *nav = job->navigator;
	ui_msg_dialog_t *dialog = NULL;
	ui_msg_dialog_params_t params;
	errno_t rc;
	int rv;

	rc = fmgt_create(&fmgt);
	if (rc != EOK) {
		/* out of memory */
		return;
	}

	fmgt_set_cb(fmgt, &rename_fmgt_cb, (void *)nav);
	fmgt_set_init_update(fmgt, true);

	rc = fmgt_rename(fmgt, job->old_path, job->new_name);
	if (rc != EOK) {
		rv = asprintf(&msg, "Error creating directory (%s).",
		    str_error(rc));
		if (rv < 0)
			return;
		goto error;
	}

	fmgt_destroy(fmgt);
	ui_lock(nav->ui);
	progress_dlg_destroy(nav->progress_dlg);
	navigator_refresh_panels(nav);
	ui_unlock(nav->ui);
	free(job);
	return;
error:
	fmgt_destroy(fmgt);
	ui_lock(nav->ui);
	progress_dlg_destroy(nav->progress_dlg);
	navigator_refresh_panels(nav);
	ui_msg_dialog_params_init(&params);
	params.caption = "Error";
	params.text = msg;
	(void) ui_msg_dialog_create(nav->ui, &params, &dialog);
	ui_unlock(nav->ui);
	free(msg);
}

/** Rename dialog confirmed.
 *
 * @param dlg Rename dialog
 * @param arg Argument (navigator_t *)
 * @param new_name New name
 */
static void rename_bok(rename_dlg_t *dlg, void *arg, const char *new_name)
{
	navigator_t *nav = (navigator_t *)arg;
	ui_msg_dialog_t *dialog = NULL;
	navigator_rename_job_t *job;
	ui_msg_dialog_params_t params;
	progress_dlg_params_t pd_params;
	char *msg = NULL;
	errno_t rc;

	job = calloc(1, sizeof(navigator_rename_job_t));
	if (job == NULL)
		return;

	job->navigator = nav;
	job->old_path = str_dup(dlg->old_path);
	job->new_name = new_name;

	if (job->old_path == NULL) {
		free(job);
		return;
	}

	rename_dlg_destroy(dlg);

	progress_dlg_params_init(&pd_params);
	pd_params.caption = "Creating new file";

	rc = progress_dlg_create(nav->ui, &pd_params, &nav->progress_dlg);
	if (rc != EOK) {
		msg = str_dup("Out of memory.");
		if (msg == NULL)
			return;
		goto error;
	}

	progress_dlg_set_cb(nav->progress_dlg, &navigator_progress_cb,
	    (void *)nav);

	rc = navigator_worker_start(nav, rename_wfunc, (void *)job);
	if (rc != EOK) {
		msg = str_dup("Out of memory.");
		if (msg == NULL)
			return;
		goto error;
	}

	return;
error:
	ui_msg_dialog_params_init(&params);
	params.caption = "Error";
	params.text = msg;
	(void) ui_msg_dialog_create(nav->ui, &params, &dialog);
	free(msg);
}

/** Rename dialog cancelled.
 *
 * @param dlg Rename dialog
 * @param arg Argument (navigator_t *)
 */
static void rename_bcancel(rename_dlg_t *dlg, void *arg)
{
	(void)arg;
	rename_dlg_destroy(dlg);
}

/** Rename dialog closed.
 *
 * @param dlg Rename dialog
 * @param arg Argument (navigator_t *)
 */
static void rename_close(rename_dlg_t *dlg, void *arg)
{
	(void)arg;
	rename_dlg_destroy(dlg);
}

/** Rename abort query.
 *
 * @param arg Argument (navigator_t *)
 * @return @c true iff abort is requested
 */
static bool rename_abort_query(void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	return nav->abort_op;
}

/** Rename progress update.
 *
 * @param arg Argument (navigator_t *)
 * @param progress Progress update
 */
static void rename_progress(void *arg, fmgt_progress_t *progress)
{
	navigator_t *nav = (navigator_t *)arg;

	progress_dlg_set_progress(nav->progress_dlg, progress);
}

/** @}
 */
