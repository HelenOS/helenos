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

/** @addtogroup fmgt
 * @{
 */
/** @file File management library.
 */

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <vfs/vfs.h>
#include <dirent.h>

#include "fmgt.h"

#define NEWNAME_LEN 64
#define BUFFER_SIZE 16384

/** Create file management library instance.
 *
 * @param rfmgt Place to store pointer to new file management library instance
 * @return EOK on succcess, ENOMEM if out of memory.
 */
errno_t fmgt_create(fmgt_t **rfmgt)
{
	fmgt_t *fmgt;

	fmgt = calloc(1, sizeof(fmgt_t));
	if (fmgt == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&fmgt->lock);
	fmgt->timer = fibril_timer_create(&fmgt->lock);
	if (fmgt->timer == NULL) {
		free(fmgt);
		return ENOMEM;
	}

	*rfmgt = fmgt;
	return EOK;
}

/** Create file management library instance.
 *
 * @param fmgt File management object
 * @param cb Callback functions
 * @param arg Argument to callback functions
 * @return EOK on succcess, ENOMEM if out of memory.
 */
void fmgt_set_cb(fmgt_t *fmgt, fmgt_cb_t *cb, void *arg)
{
	fmgt->cb = cb;
	fmgt->cb_arg = arg;
}

/** Configure whether to give immediate initial progress update.
 *
 * @param fmgt File management object
 * @param enabled @c true to post and immediate initial progress update
 */
void fmgt_set_init_update(fmgt_t *fmgt, bool enabled)
{
	fmgt->do_init_update = enabled;
}

/** Destroy file management library instance.
 *
 * @param fmgt File management object
 */
void fmgt_destroy(fmgt_t *fmgt)
{
	(void)fibril_timer_clear(fmgt->timer);
	fibril_timer_destroy(fmgt->timer);
	free(fmgt);
}

/** Suggest file name for new file.
 *
 * @param fmgt File management object
 * @param rstr Place to store pointer to newly allocated string
 * @return EOK on success or an error code
 */
errno_t fmgt_new_file_suggest(char **rstr)
{
	errno_t rc;
	vfs_stat_t stat;
	unsigned u;
	char name[NEWNAME_LEN];

	u = 0;
	while (true) {
		snprintf(name, sizeof(name), "noname%02u.txt", u);
		rc = vfs_stat_path(name, &stat);
		if (rc != EOK)
			break;

		++u;
	}

	*rstr = str_dup(name);
	if (*rstr == NULL)
		return ENOMEM;

	return EOK;
}

/** Get progress update report.
 *
 * @param fmgt File management object
 * @param progress Place to store progress update
 */
static void fmgt_get_progress(fmgt_t *fmgt, fmgt_progress_t *progress)
{
	unsigned percent;

	if (fmgt->curf_totalb > 0)
		percent = fmgt->curf_procb * 100 / fmgt->curf_totalb;
	else
		percent = 100;

	capa_blocks_format_buf(fmgt->curf_procb, 1, progress->curf_procb,
	    sizeof(progress->curf_procb));
	capa_blocks_format_buf(fmgt->curf_totalb, 1, progress->curf_totalb,
	    sizeof(progress->curf_totalb));
	snprintf(progress->curf_percent, sizeof(progress->curf_percent), "%u%%",
	    percent);
}

/** Give the caller progress update.
 *
 * @param fmgt File management object
 */
static void fmgt_progress_update(fmgt_t *fmgt)
{
	fmgt_progress_t progress;

	if (fmgt->cb != NULL && fmgt->cb->progress != NULL) {
		fmgt_get_progress(fmgt, &progress);
		fmgt->curf_progr = true;
		fmgt->cb->progress(fmgt->cb_arg, &progress);
	}
}

/** Provide initial progress update (if required).
 *
 * The caller configures the file management object regarding whether
 * initial updates are required.
 *
 * @param fmgt File management object
 */
static void fmgt_initial_progress_update(fmgt_t *fmgt)
{
	if (fmgt->do_init_update)
		fmgt_progress_update(fmgt);
}

/** Provide final progress update (if required).
 *
 * Final update is provided only if a previous progress update was given.
 *
 * @param fmgt File management object
 */
static void fmgt_final_progress_update(fmgt_t *fmgt)
{
	if (fmgt->curf_progr)
		fmgt_progress_update(fmgt);
}

/** Progress timer function.
 *
 * Periodically called to provide progress updates.
 *
 * @param arg Argument (fmgt_t *)
 */
static void fmgt_timer_fun(void *arg)
{
	fmgt_t *fmgt = (fmgt_t *)arg;

	fmgt_progress_update(fmgt);
	fibril_timer_set(fmgt->timer, 500000, fmgt_timer_fun, (void *)fmgt);
}

/** Start progress update timer.
 *
 * @param fmgt File management object
 */
static void fmgt_timer_start(fmgt_t *fmgt)
{
	fibril_timer_set(fmgt->timer, 500000, fmgt_timer_fun, (void *)fmgt);
}

/** Query caller whether operation should be aborted.
 *
 * @param fmgt File management object
 * @return @c true iff operation should be aborted
 */
static bool fmgt_abort_query(fmgt_t *fmgt)
{
	if (fmgt->cb != NULL && fmgt->cb->abort_query!= NULL)
		return fmgt->cb->abort_query(fmgt->cb_arg);
	else
		return false;
}

/** Create new file.
 *
 * @param fmgt File management object
 * @param fname File name
 * @param fsize Size of new file (number of zero bytes to fill in)
 * @param flags New file flags
 * @return EOK on success or an error code
 */
errno_t fmgt_new_file(fmgt_t *fmgt, const char *fname, uint64_t fsize,
    fmgt_nf_flags_t flags)
{
	int fd;
	size_t nw;
	aoff64_t pos = 0;
	uint64_t now;
	char *buffer;
	errno_t rc;

	buffer = calloc(BUFFER_SIZE, 1);
	if (buffer == NULL)
		return ENOMEM;

	rc = vfs_lookup_open(fname, WALK_REGULAR | WALK_MUST_CREATE,
	    MODE_WRITE, &fd);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}

	fmgt->curf_procb = 0;
	fmgt->curf_totalb = fsize;
	fmgt->curf_progr = false;
	fmgt_timer_start(fmgt);

	fmgt_initial_progress_update(fmgt);

	if ((flags & nf_sparse) != 0) {
		fmgt->curf_procb = fsize - 1;
		pos = fsize - 1;
	}

	while (fmgt->curf_procb < fsize) {
		now = fsize - fmgt->curf_procb;
		if (now > BUFFER_SIZE)
			now = BUFFER_SIZE;

		rc = vfs_write(fd, &pos, buffer, now, &nw);
		if (rc != EOK) {
			free(buffer);
			vfs_put(fd);
			fmgt_final_progress_update(fmgt);
			return rc;
		}

		fmgt->curf_procb += nw;

		/* User requested abort? */
		if (fmgt_abort_query(fmgt)) {
			free(buffer);
			vfs_put(fd);
			fmgt_final_progress_update(fmgt);
			return EINTR;
		}
	}

	free(buffer);
	vfs_put(fd);
	fmgt_final_progress_update(fmgt);
	return EOK;
}

/** @}
 */
