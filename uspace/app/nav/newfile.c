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

#include <capa.h>
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
#include "dlg/newfiledlg.h"
#include "dlg/progress.h"
#include "menu.h"
#include "newfile.h"
#include "nav.h"
#include "types/newfile.h"

static void new_file_bok(new_file_dlg_t *, void *, const char *, const char *,
    bool);
static void new_file_bcancel(new_file_dlg_t *, void *);
static void new_file_close(new_file_dlg_t *, void *);

static new_file_dlg_cb_t new_file_cb = {
	.bok = new_file_bok,
	.bcancel = new_file_bcancel,
	.close = new_file_close
};

static bool new_file_abort_query(void *);
static void new_file_progress(void *, fmgt_progress_t *);

static fmgt_cb_t new_file_fmgt_cb = {
	.abort_query = new_file_abort_query,
	.progress = new_file_progress,
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

/** New file worker function.
 *
 * @param arg Argument (navigator_new_file_job_t)
 */
static void new_file_wfunc(void *arg)
{
	fmgt_t *fmgt = NULL;
	navigator_new_file_job_t *job = (navigator_new_file_job_t *)arg;
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

	fmgt_set_cb(fmgt, &new_file_fmgt_cb, (void *)nav);
	fmgt_set_init_update(fmgt, true);

	rc = fmgt_new_file(fmgt, job->fname, job->nbytes,
	    job->sparse ? nf_sparse : nf_none);
	if (rc != EOK) {
		rv = asprintf(&msg, "Error creating file (%s).",
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

/** New file dialog confirmed.
 *
 * @param dlg New file dialog
 * @param arg Argument (navigator_t *)
 * @param fname New file name
 * @param fsize New file size
 * @param sparse @c true to create a sparse file
 */
static void new_file_bok(new_file_dlg_t *dlg, void *arg, const char *fname,
    const char *fsize, bool sparse)
{
	navigator_t *nav = (navigator_t *)arg;
	ui_msg_dialog_t *dialog = NULL;
	navigator_new_file_job_t *job;
	ui_msg_dialog_params_t params;
	progress_dlg_params_t pd_params;
	capa_spec_t fcap;
	char *msg = NULL;
	errno_t rc;
	uint64_t nbytes;
	int rv;

	rc = capa_parse(fsize, &fcap);
	if (rc != EOK) {
		/* invalid file size */
		return;
	}

	new_file_dlg_destroy(dlg);

	rc = capa_to_blocks(&fcap, cv_nom, 1, &nbytes);
	if (rc != EOK) {
		rv = asprintf(&msg, "File size too large (%s).", fsize);
		if (rv < 0)
			return;
		goto error;
	}

	job = calloc(1, sizeof(navigator_new_file_job_t));
	if (job == NULL)
		return;

	job->navigator = nav;
	job->fname = fname;
	job->nbytes = nbytes;
	job->sparse = sparse;

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

	rc = navigator_worker_start(nav, new_file_wfunc, (void *)job);
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

/** New file abort query.
 *
 * @param arg Argument (navigator_t *)
 * @return @c true iff abort is requested
 */
static bool new_file_abort_query(void *arg)
{
	navigator_t *nav = (navigator_t *)arg;

	return nav->abort_op;
}

/** New file progress update.
 *
 * @param arg Argument (navigator_t *)
 * @param progress Progress update
 */
static void new_file_progress(void *arg, fmgt_progress_t *progress)
{
	navigator_t *nav = (navigator_t *)arg;
	char buf[128];

	snprintf(buf, sizeof(buf), "Written %s of %s (%s done).",
	    progress->curf_procb, progress->curf_totalb,
	    progress->curf_percent);
	progress_dlg_set_curf_prog(nav->progress_dlg, buf);
}

/** @}
 */
