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
 * @file Navigator Delete Files.
 */

#include <fmgt.h>
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
#include "dlg/progress.h"
#include "dlg/deletedlg.h"
#include "menu.h"
#include "nav.h"
#include "types/delete.h"
#include "delete.h"

static void delete_bok(delete_dlg_t *, void *);
static void delete_bcancel(delete_dlg_t *, void *);
static void delete_close(delete_dlg_t *, void *);

static delete_dlg_cb_t delete_cb = {
	.bok = delete_bok,
	.bcancel = delete_bcancel,
	.close = delete_close
};

static bool delete_abort_query(void *);
static void delete_progress(void *, fmgt_progress_t *);

static fmgt_cb_t delete_fmgt_cb = {
	.abort_query = delete_abort_query,
	.io_error_query = navigator_io_error_query,
	.progress = delete_progress,
};

/** Open Delete dialog.
 *
 * @param navigator Navigator
 * @param flist File list
 */
void navigator_delete_dlg(navigator_t *navigator, fmgt_flist_t *flist)
{
	delete_dlg_t *dlg;
	errno_t rc;

	rc = delete_dlg_create(navigator->ui, flist, &dlg);
	if (rc != EOK)
		return;

	delete_dlg_set_cb(dlg, &delete_cb, (void *)navigator);
}

/** Delete worker function.
 *
 * @param arg Argument (navigator_delete_job_t)
 */
static void delete_wfunc(void *arg)
{
	fmgt_t *fmgt = NULL;
	navigator_delete_job_t *job = (navigator_delete_job_t *)arg;
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

	fmgt_set_cb(fmgt, &delete_fmgt_cb, (void *)nav);
	fmgt_set_init_update(fmgt, true);

	rc = fmgt_delete(fmgt, job->flist);
	if (rc != EOK) {
		rv = asprintf(&msg, "Error deleteing file(s) (%s).",
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
	fmgt_flist_destroy(job->flist);
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

/** Delete dialog confirmed.
 *
 * @param dlg Delete dialog
 * @param arg Argument (navigator_t *)
 */
static void delete_bok(delete_dlg_t *dlg, void *arg)
{
	navigator_t *nav = (navigator_t *)arg;
	ui_msg_dialog_t *dialog = NULL;
	navigator_delete_job_t *job;
	ui_msg_dialog_params_t params;
	progress_dlg_params_t pd_params;
	char *msg = NULL;
	errno_t rc;

	delete_dlg_destroy(dlg);

	job = calloc(1, sizeof(navigator_delete_job_t));
	if (job == NULL)
		return;

	job->navigator = nav;
	job->flist = dlg->flist;
	dlg->flist = NULL;

	progress_dlg_params_init(&pd_params);
	pd_params.caption = "Deleting";

	rc = progress_dlg_create(nav->ui, &pd_params, &nav->progress_dlg);
	if (rc != EOK) {
		msg = str_dup("Out of memory.");
		if (msg == NULL)
			return;
		goto error;
	}

	progress_dlg_set_cb(nav->progress_dlg, &navigator_progress_cb,
	    (void *)nav);

	rc = navigator_worker_start(nav, delete_wfunc, (void *)job);
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

/** New file dialog cancelled.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 */
static void delete_bcancel(delete_dlg_t *dlg, void *arg)
{
	(void)arg;
	delete_dlg_destroy(dlg);
}

/** New file dialog closed.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 */
static void delete_close(delete_dlg_t *dlg, void *arg)
{
	(void)arg;
	delete_dlg_destroy(dlg);
}

/** New file abort query.
 *
 * @param arg Argument (navigator_t *)
 * @return @c true iff abort is requested
 */
static bool delete_abort_query(void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	return nav->abort_op;
}

/** New file progress update.
 *
 * @param arg Argument (navigator_t *)
 * @param progress Progress update
 */
static void delete_progress(void *arg, fmgt_progress_t *progress)
{
	navigator_t *nav = (navigator_t *)arg;

	progress_dlg_set_progress(nav->progress_dlg, progress);
}

/** @}
 */
