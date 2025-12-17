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
/** @file Copy files and directories.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "fmgt/walk.h"
#include "../private/fmgt.h"

static errno_t fmgt_copy_dir_enter(void *, const char *, const char *);
static errno_t fmgt_copy_file(void *, const char *, const char *);

static fmgt_walk_cb_t fmgt_copy_cb = {
	.dir_enter = fmgt_copy_dir_enter,
	.file = fmgt_copy_file
};

static errno_t fmgt_write(fmgt_t *fmgt, int fd, const char *fname,
    aoff64_t *pos, void *buffer, size_t nbytes)
{
	size_t total_written;
	char *bp = (char *)buffer;
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	size_t nw;
	errno_t rc;

	total_written = 0;
	while (total_written < nbytes) {
		do {
			rc = vfs_write(fd, pos, bp + total_written,
			    nbytes - total_written, &nw);
			if (rc == EOK)
				break;

			/* I/O error */
			err.fname = fname;
			err.optype = fmgt_io_write;
			err.rc = rc;
			fmgt_timer_stop(fmgt);
			action = fmgt_io_error_query(fmgt, &err);
			fmgt_timer_start(fmgt);
		} while (action == fmgt_er_retry);

		/* Not recovered? */
		if (rc != EOK)
			return rc;

		total_written += nw;
	}

	return EOK;
}

/** Copy operation - enter directory.
 *
 * @param arg Argument (fmgt_t *)
 * @param fname Source directory name
 * @param dest Destination directory name
 * @return EOK on success or an error code
 */
static errno_t fmgt_copy_dir_enter(void *arg, const char *src, const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)arg;
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	do {
		rc = vfs_link_path(dest, KIND_DIRECTORY, NULL);

		/* It is okay if the directory exists. */
		if (rc == EOK || rc == EEXIST)
			break;

		/* I/O error */
		err.fname = dest;
		err.optype = fmgt_io_create;
		err.rc = rc;

		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	return rc;
}

/** Copy single file.
 *
 * @param arg Argument (fmgt_t *)
 * @param fname Source file name
 * @param dest Destination file name
 * @return EOK on success or an error code
 */
static errno_t fmgt_copy_file(void *arg, const char *src, const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)arg;
	int rfd;
	int wfd;
	size_t nr;
	aoff64_t rpos = 0;
	aoff64_t wpos = 0;
	char *buffer;
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	buffer = calloc(BUFFER_SIZE, 1);
	if (buffer == NULL)
		return ENOMEM;

	do {
		rc = vfs_lookup_open(src, WALK_REGULAR, MODE_READ, &rfd);
		if (rc == EOK)
			break;

		/* I/O error */
		err.fname = src;
		err.optype = fmgt_io_open;
		err.rc = rc;
		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	/* Not recovered? */
	if (rc != EOK) {
		free(buffer);
		return rc;
	}

	do {
		rc = vfs_lookup_open(dest, WALK_REGULAR | WALK_MAY_CREATE,
		    MODE_WRITE, &wfd);
		if (rc == EOK)
			break;

		/* I/O error */
		err.fname = dest;
		err.optype = fmgt_io_create;
		err.rc = rc;
		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	if (rc != EOK) {
		free(buffer);
		vfs_put(rfd);
		return rc;
	}

	fmgt_progress_init_file(fmgt, src);

	do {
		do {
			rc = vfs_read(rfd, &rpos, buffer, BUFFER_SIZE, &nr);
			if (rc == EOK)
				break;

			/* I/O error */
			err.fname = src;
			err.optype = fmgt_io_read;
			err.rc = rc;
			fmgt_timer_stop(fmgt);
			action = fmgt_io_error_query(fmgt, &err);
			fmgt_timer_start(fmgt);
		} while (action == fmgt_er_retry);

		/* Not recovered? */
		if (rc != EOK)
			goto error;

		rc = fmgt_write(fmgt, wfd, dest, &wpos, buffer, nr);
		if (rc != EOK)
			goto error;

		fmgt_progress_incr_bytes(fmgt, nr);

		/* User requested abort? */
		if (fmgt_abort_query(fmgt)) {
			rc = EINTR;
			goto error;
		}
	} while (nr > 0);

	free(buffer);
	vfs_put(rfd);
	vfs_put(wfd);
	fmgt_progress_incr_files(fmgt);
	return EOK;
error:
	free(buffer);
	vfs_put(rfd);
	vfs_put(wfd);
	fmgt_final_progress_update(fmgt);
	return rc;
}

/** copy files.
 *
 * @param fmgt File management object
 * @param flist File list
 * @param dest Destination path
 * @return EOK on success or an error code
 */
errno_t fmgt_copy(fmgt_t *fmgt, fmgt_flist_t *flist, const char *dest)
{
	fmgt_walk_params_t params;
	errno_t rc;

	fmgt_walk_params_init(&params);

	params.flist = flist;
	params.dest = dest;
	params.cb = &fmgt_copy_cb;
	params.arg = (void *)fmgt;
	if (fmgt_is_dir(dest))
		params.into_dest = true;

	fmgt_progress_init(fmgt);

	fmgt_timer_start(fmgt);
	fmgt_initial_progress_update(fmgt);
	rc = fmgt_walk(&params);
	fmgt_final_progress_update(fmgt);
	return rc;
}

/** @}
 */
