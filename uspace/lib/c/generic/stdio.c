/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file C standard file manipulation functions
 */

#include <errno.h>
#include <stdio.h>
#include <str_error.h>
#include <vfs/vfs.h>

/** Get stream position.
 *
 * @param stream Stream
 * @param pos Place to store position
 *
 * @return Zero on success, non-zero on failure
 */
int fgetpos(FILE *stream, fpos_t *pos)
{
	off64_t p;

	p = ftell64(stream);
	if (p < 0)
		return -1;

	pos->pos = p;
	return 0;
}

/** Get stream position.
 *
 * @param stream Stream
 * @param pos Position
 *
 * @return Zero on sucess, non-zero on failure
 */
int fsetpos(FILE *stream, const fpos_t *pos)
{
	int rc;

	rc = fseek64(stream, pos->pos, SEEK_SET);
	if (rc < 0)
		return -1;

	return 0;
}

/** Rename file or directory (C standard) */
int rename(const char *old_path, const char *new_path)
{
	errno_t rc;

	rc = vfs_rename_path(old_path, new_path);
	if (rc != EOK) {
		errno = rc;
		return -1;
	}

	return 0;
}

/** Remove file or directory (C standard) */
int remove(const char *path)
{
	errno_t rc;

	rc = vfs_unlink_path(path);
	if (rc != EOK) {
		errno = rc;
		return -1;
	}

	return 0;
}

/** Print error message and string representation of @c errno.
 *
 * @param s Error message
 */
void perror(const char *s)
{
	if (s != NULL && *s != '\0')
		fprintf(stderr, "%s: %s\n", s, str_error(errno));
	else
		fprintf(stderr, "%s\n", str_error(errno));
}

/** @}
 */
