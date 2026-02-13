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
/** @file Move files and directories.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "fmgt/walk.h"
#include "../private/fmgt.h"
#include "../private/fsops.h"

static errno_t fmgt_delete_file(fmgt_walk_t *, const char *, const char *);
static errno_t fmgt_delete_dir_leave(fmgt_walk_t *, const char *, const char *);

static fmgt_walk_cb_t fmgt_delete_cb = {
	.file = fmgt_delete_file,
	.dir_leave = fmgt_delete_dir_leave
};

/** Delete single file.
 *
 * @param walk Walk
 * @param fname Source file name
 * @param dest Destination file name
 * @return EOK on success or an error code
 */
static errno_t fmgt_delete_file(fmgt_walk_t *walk, const char *src,
    const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)walk->params->arg;
	errno_t rc;

	/* Remove original file. */
	rc = fmgt_remove(fmgt, src);
	if (rc != EOK)
		return rc;

	fmgt_progress_incr_files(fmgt);
	return EOK;
}

/** Delete operation - leave directory.
 *
 * @param walk Walk
 * @param fname Source directory name
 * @param dest Destination directory name
 * @return EOK on success or an error code
 */
static errno_t fmgt_delete_dir_leave(fmgt_walk_t *walk, const char *src,
    const char *dest)
{
	fmgt_t *fmgt = (fmgt_t *)walk->params->arg;
	(void)dest;
	return fmgt_remove(fmgt, src);
}

/** Delete files.
 *
 * @param fmgt File management object
 * @param flist File list
 * @return EOK on success or an error code
 */
errno_t fmgt_delete(fmgt_t *fmgt, fmgt_flist_t *flist)
{
	fmgt_walk_params_t params;
	errno_t rc;

	fmgt_walk_params_init(&params);

	params.flist = flist;
	params.cb = &fmgt_delete_cb;
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
