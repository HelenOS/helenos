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
 * @file Navigator Copy Files.
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
#include "copy.h"
#include "dlg/progress.h"
#include "dlg/copydlg.h"
#include "menu.h"
#include "nav.h"
#include "types/copy.h"
#include "panel.h"

static void copy_bok(copy_dlg_t *, void *);
static void copy_bcancel(copy_dlg_t *, void *);
static void copy_close(copy_dlg_t *, void *);

static copy_dlg_cb_t copy_cb = {
	.bok = copy_bok,
	.bcancel = copy_bcancel,
	.close = copy_close
};

static bool copy_abort_query(void *);
static void copy_progress(void *, fmgt_progress_t *);

static fmgt_cb_t copy_fmgt_cb = {
	.abort_query = copy_abort_query,
	.io_error_query = navigator_io_error_query,
	.exists_query = navigator_exists_query,
	.progress = copy_progress,
};

/** Open Copy dialog.
 *
 * @param navigator Navigator
 * @param flist File list
 */
void navigator_copy_dlg(navigator_t *navigator, fmgt_flist_t *flist)
{
	copy_dlg_t *dlg;
	panel_t *dpanel;
	char *dest;
	errno_t rc;

	/* Get destination panel. */
	dpanel = navigator_get_inactive_panel(navigator);
	if (dpanel == NULL) {
		/* out of memory */
		return;
	}

	/* Get destination path from destination panel. */
	dest = panel_get_dir(dpanel);
	if (dest == NULL) {
		/* out of memory */
		return;
	}

	rc = copy_dlg_create(navigator->ui, flist, dest, &dlg);
	if (rc != EOK) {
		free(dest);
		return;
	}

	copy_dlg_set_cb(dlg, &copy_cb, (void *)navigator);
	free(dest);
}

/** Copy worker function.
 *
 * @param arg Argument (navigator_copy_job_t)
 */
static void copy_wfunc(void *arg)
{
	fmgt_t *fmgt = NULL;
	navigator_copy_job_t *job = (navigator_copy_job_t *)arg;
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

	fmgt_set_cb(fmgt, &copy_fmgt_cb, (void *)nav);
	fmgt_set_init_update(fmgt, true);

	rc = fmgt_copy(fmgt, job->flist, job->dest);
	if (rc != EOK) {
		rv = asprintf(&msg, "Error copying file(s) (%s).",
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

/** Copy dialog confirmed.
 *
 * @param dlg Copy dialog
 * @param arg Argument (navigator_t *)
 */
static void copy_bok(copy_dlg_t *dlg, void *arg)
{
	navigator_t *nav = (navigator_t *)arg;
	ui_msg_dialog_t *dialog = NULL;
	navigator_copy_job_t *job;
	ui_msg_dialog_params_t params;
	progress_dlg_params_t pd_params;
	char *msg = NULL;
	errno_t rc;

	job = calloc(1, sizeof(navigator_copy_job_t));
	if (job == NULL)
		return;

	job->navigator = nav;
	job->flist = dlg->flist;
	job->dest = str_dup(ui_entry_get_text(dlg->edest));
	if (job->dest == NULL) {
		free(job);
		return;
	}

	copy_dlg_destroy(dlg);
	dlg->flist = NULL;

	progress_dlg_params_init(&pd_params);
	pd_params.caption = "Copying";

	rc = progress_dlg_create(nav->ui, &pd_params, &nav->progress_dlg);
	if (rc != EOK) {
		msg = str_dup("Out of memory.");
		if (msg == NULL)
			return;
		goto error;
	}

	progress_dlg_set_cb(nav->progress_dlg, &navigator_progress_cb,
	    (void *)nav);

	rc = navigator_worker_start(nav, copy_wfunc, (void *)job);
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
static void copy_bcancel(copy_dlg_t *dlg, void *arg)
{
	(void)arg;
	copy_dlg_destroy(dlg);
}

/** New file dialog closed.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 */
static void copy_close(copy_dlg_t *dlg, void *arg)
{
	(void)arg;
	copy_dlg_destroy(dlg);
}

/** New file abort query.
 *
 * @param arg Argument (navigator_t *)
 * @return @c true iff abort is requested
 */
static bool copy_abort_query(void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	return nav->abort_op;
}

/** New file progress update.
 *
 * @param arg Argument (navigator_t *)
 * @param progress Progress update
 */
static void copy_progress(void *arg, fmgt_progress_t *progress)
{
	navigator_t *nav = (navigator_t *)arg;

	progress_dlg_set_progress(nav->progress_dlg, progress);
}

/** @}
 */
