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
/** @file Verify files.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "fmgt/walk.h"
#include "../private/fmgt.h"

static errno_t fmgt_verify_file(fmgt_walk_t *, const char *, const char *);

static fmgt_walk_cb_t fmgt_verify_cb = {
	.file = fmgt_verify_file
};

/** Verify a single file.
 *
 * @param walk Walk
 * @param fname File name
 * @param unused Unused
 * @return EOK on success or an error code
 */
static errno_t fmgt_verify_file(fmgt_walk_t *walk, const char *fname,
    const char *unused)
{
	fmgt_t *fmgt = (fmgt_t *)walk->params->arg;
	int fd;
	size_t nr;
	aoff64_t pos = 0;
	char *buffer;
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	(void)unused;

	buffer = calloc(BUFFER_SIZE, 1);
	if (buffer == NULL)
		return ENOMEM;

	rc = vfs_lookup_open(fname, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}

	fmgt_progress_init_file(fmgt, fname);

	do {
		do {
			rc = vfs_read(fd, &pos, buffer, BUFFER_SIZE, &nr);
			if (rc == EOK)
				break;

			/* I/O error */
			err.fname = fname;
			err.optype = fmgt_io_read;
			err.rc = rc;
			fmgt_timer_stop(fmgt);
			action = fmgt_io_error_query(fmgt, &err);
			fmgt_timer_start(fmgt);
		} while (action == fmgt_er_retry);

		/* Not recovered? */
		if (rc != EOK) {
			free(buffer);
			vfs_put(fd);
			fmgt_final_progress_update(fmgt);
			return rc;
		}

		fmgt_progress_incr_bytes(fmgt, nr);

		/* User requested abort? */
		if (fmgt_abort_query(fmgt)) {
			free(buffer);
			vfs_put(fd);
			fmgt_final_progress_update(fmgt);
			return EINTR;
		}
	} while (nr > 0);

	free(buffer);
	vfs_put(fd);
	fmgt_progress_incr_files(fmgt);
	return EOK;
}

/** Verify files.
 *
 * @param fmgt File management object
 * @param flist File list
 * @return EOK on success or an error code
 */
errno_t fmgt_verify(fmgt_t *fmgt, fmgt_flist_t *flist)
{
	fmgt_walk_params_t params;
	errno_t rc;

	fmgt_walk_params_init(&params);

	params.flist = flist;
	params.cb = &fmgt_verify_cb;
	params.arg = (void *)fmgt;

	fmgt_progress_init(fmgt);

	fmgt_timer_start(fmgt);
	fmgt_initial_progress_update(fmgt);
	rc = fmgt_walk(&params);
	fmgt_final_progress_update(fmgt);
	return rc;
}

/** @}
 */
