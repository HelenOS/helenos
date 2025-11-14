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
/** @file File management library - creating new files.
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
#include "../private/fmgt.h"

#define NEWNAME_LEN 64

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
	fmgt_io_error_t err;
	fmgt_error_action_t action;
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

	/* Create sparse file? */
	if ((flags & nf_sparse) != 0) {
		fmgt->curf_procb = fsize - 1;
		pos = fsize - 1;
	}

	while (fmgt->curf_procb < fsize) {
		now = fsize - fmgt->curf_procb;
		if (now > BUFFER_SIZE)
			now = BUFFER_SIZE;

		do {
			rc = vfs_write(fd, &pos, buffer, now, &nw);
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
