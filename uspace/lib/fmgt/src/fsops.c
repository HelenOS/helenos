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
/** @file File system operations.
 *
 * These provide wrappers over normal file system operations that,
 * in addition to basic functionality, query the user how to proceed
 * in case of I/O error or if the destination file exists.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "../private/fmgt.h"
#include "../private/fsops.h"

/** Open file.
 *
 * @param fmgt File management object
 * @param fname File name
 * @param rfd Place to store file descriptor
 * @return EOK on success or an error code
 */
errno_t fmgt_open(fmgt_t *fmgt, const char *fname, int *rfd)
{
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	do {
		rc = vfs_lookup_open(fname, WALK_REGULAR, MODE_READ, rfd);
		if (rc == EOK)
			break;

		/* I/O error */
		err.fname = fname;
		err.optype = fmgt_io_open;
		err.rc = rc;
		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	return rc;
}

/** Create file.
 *
 * @param fmgt File management object
 * @param fname Destination file name
 * @param rfd Place to store file descriptor
 * @param raction If return value is EEXIST, fills in the action to take.
 * @return EOK on success or an error code
 */
errno_t fmgt_create_file(fmgt_t *fmgt, const char *fname, int *rfd,
    fmgt_exists_action_t *raction)
{
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	fmgt_exists_t exists;
	fmgt_exists_action_t exaction;
	bool may_overwrite = false;
	unsigned flags;
	errno_t rc;

	do {
		flags = WALK_REGULAR | (may_overwrite ? WALK_MAY_CREATE :
		    WALK_MUST_CREATE);
		rc = vfs_lookup_open(fname, flags, MODE_WRITE, rfd);
		if (rc == EOK)
			break;

		if (rc == EEXIST) {
			/* File exists */
			exists.fname = fname;
			fmgt_timer_stop(fmgt);
			exaction = fmgt_exists_query(fmgt, &exists);
			fmgt_timer_start(fmgt);

			*raction = exaction;
			if (exaction != fmgt_exr_overwrite)
				break;

			may_overwrite = true;
		} else {
			/* I/O error */
			err.fname = fname;
			err.optype = fmgt_io_create;
			err.rc = rc;
			fmgt_timer_stop(fmgt);
			action = fmgt_io_error_query(fmgt, &err);
			fmgt_timer_start(fmgt);

			if (action != fmgt_er_retry)
				break;
		}
	} while (true);

	return rc;
}

/** Create directory.
 *
 * @param fmgt File management object
 * @param dname Directory name
 * @return EOK on success or an error code
 */
errno_t fmgt_create_dir(fmgt_t *fmgt, const char *dname)
{
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	do {
		rc = vfs_link_path(dname, KIND_DIRECTORY, NULL);

		/* It is okay if the directory exists. */
		if (rc == EOK || rc == EEXIST)
			break;

		/* I/O error */
		err.fname = dname;
		err.optype = fmgt_io_create;
		err.rc = rc;

		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	if (rc == EEXIST)
		return EOK;

	return rc;
}

/** Remove file or empty directory.
 *
 * @param fmgt File management object
 * @param fame File or directory name
 * @return EOK on success or an error code
 */
errno_t fmgt_remove(fmgt_t *fmgt, const char *fname)
{
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	do {
		rc = vfs_unlink_path(fname);
		if (rc == EOK)
			break;

		/* I/O error */
		err.fname = fname;
		err.optype = fmgt_io_delete;
		err.rc = rc;

		fmgt_timer_stop(fmgt);
		action = fmgt_io_error_query(fmgt, &err);
		fmgt_timer_start(fmgt);
	} while (action == fmgt_er_retry);

	return rc;
}

/** Read data from file.
 *
 * @param fmgt File management object
 * @param fd File descriptor
 * @param fname File name (for printing diagnostics)
 * @param pos Pointer to current position (will be updated)
 * @param buffer Data buffer
 * @param nbytes Number of bytes to read
 * @param nr Place to store number of bytes read
 * @return EOK on success or an error code
 */
errno_t fmgt_read(fmgt_t *fmgt, int fd, const char *fname, aoff64_t *pos,
    void *buffer, size_t nbytes, size_t *nr)
{
	char *bp = (char *)buffer;
	fmgt_io_error_t err;
	fmgt_error_action_t action;
	errno_t rc;

	do {
		rc = vfs_read(fd, pos, bp, nbytes, nr);
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
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Write data to file.
 *
 * @param fmgt File management object
 * @param fd File descriptor
 * @param fname File name (for printing diagnostics)
 * @param pos Pointer to current position (will be updated)
 * @param buffer Pointer to data
 * @param nbytes Number of bytes to write
 * @return EOK on success or an error code
 */
errno_t fmgt_write(fmgt_t *fmgt, int fd, const char *fname, aoff64_t *pos,
    void *buffer, size_t nbytes)
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

/** @}
 */
