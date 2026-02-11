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
#include "../private/fsops.h"

static errno_t fmgt_copy_dir_enter(fmgt_walk_t *, const char *, const char *);
static errno_t fmgt_copy_file(fmgt_walk_t *, const char *, const char *);

static fmgt_walk_cb_t fmgt_copy_cb = {
	.dir_enter = fmgt_copy_dir_enter,
	.file = fmgt_copy_file
};

/** Copy operation - enter directory.
 *
 * @param walk Walk
 * @param fname Source directory name
 * @param dest Destination directory name
 * @return EOK on success or an error code
 */
static errno_t fmgt_copy_dir_enter(fmgt_walk_t *walk, const char *src,
    const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)walk->params->arg;

	(void)dest;
	return fmgt_create_dir(fmgt, dest, false);
}

/** Copy single file.
 *
 * @param walk Walk
 * @param fname Source file name
 * @param dest Destination file name
 * @return EOK on success or an error code
 */
static errno_t fmgt_copy_file(fmgt_walk_t *walk, const char *src,
    const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)walk->params->arg;
	fmgt_exists_action_t exaction;
	int rfd;
	int wfd;
	size_t nr;
	aoff64_t rpos = 0;
	aoff64_t wpos = 0;
	char *buffer;
	errno_t rc;

	buffer = calloc(BUFFER_SIZE, 1);
	if (buffer == NULL)
		return ENOMEM;

	rc = fmgt_open(fmgt, src, &rfd);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}

	rc = fmgt_create_file(fmgt, dest, &wfd, &exaction);
	if (rc != EOK) {
		free(buffer);
		vfs_put(rfd);

		if (rc == EEXIST && exaction != fmgt_exr_fail) {
			if (exaction == fmgt_exr_abort)
				walk->stop = true;
			return EOK;
		}

		return rc;
	}

	fmgt_progress_init_file(fmgt, src);

	do {
		rc = fmgt_read(fmgt, rfd, src, &rpos, buffer, BUFFER_SIZE,
		    &nr);
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

/** Copy files.
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
