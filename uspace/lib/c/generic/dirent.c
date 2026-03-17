/*
 * Copyright (c) 2008 Jakub Jermar
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
/** @file
 */

#include <vfs/vfs.h>
#include <stdlib.h>
#include <dirent.h>
#include <stddef.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

struct __dirstream {
	int fd;
	struct dirent res;
	aoff64_t pos;
};

/** Like opendir_handle, but takes ownership of the handle if successful. */
static DIR *opendir_internal(int handle)
{
	DIR *dirp = malloc(sizeof(DIR));
	if (!dirp) {
		errno = ENOMEM;
		return NULL;
	}

	errno_t rc = vfs_open(handle, MODE_READ);
	if (rc != EOK) {
		free(dirp);
		errno = rc;
		return NULL;
	}

	dirp->fd = handle;
	dirp->pos = 0;
	return dirp;
}

/** Open directory by its handle (a.k.a. file descriptor).
 *
 * @param handle Directory handle
 * @return Non-NULL pointer on success. On error returns @c NULL and sets errno.
 */
DIR *opendir_handle(int handle)
{
	int my_handle;
	// Clone the file handle, otherwise closedir would put the
	// handle that was passed to us here by the caller and that we don't own.
	errno_t rc = vfs_clone(handle, -1, false, &my_handle);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}

	DIR *dirp = opendir_internal(my_handle);
	if (!dirp) {
		rc = errno;
		vfs_put(my_handle);
		errno = rc;
		return NULL;
	}
	return dirp;
}

/** Open directory by its pathname.
 *
 * @param dirname Directory pathname
 *
 * @return Non-NULL pointer on success. On error returns @c NULL and sets errno.
 */
DIR *opendir(const char *dirname)
{
	int fd;
	errno_t rc = vfs_lookup(dirname, WALK_DIRECTORY, &fd);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}

	DIR *dirp = opendir_internal(fd);
	if (!dirp) {
		rc = errno;
		vfs_put(fd);
		errno = rc;
		return NULL;
	}
	return dirp;
}

/** Read current directory entry and advance to the next one.
 *
 * @param dirp Open directory
 * @return Non-NULL pointer to directory entry on success. On error returns
 *         @c NULL and sets errno.
 */
struct dirent *readdir(DIR *dirp)
{
	errno_t rc;
	ssize_t len = 0;

	rc = vfs_read_short(dirp->fd, dirp->pos, dirp->res.d_name,
	    sizeof(dirp->res.d_name), &len);
	if (rc != EOK) {
		errno = rc;
		return NULL;
	}

	assert(strnlen(dirp->res.d_name, sizeof(dirp->res.d_name)) < sizeof(dirp->res.d_name));

	dirp->pos += len;

	return &dirp->res;
}

/** Rewind directory position to the beginning.
 *
 * @param dirp Open directory
 */
void rewinddir(DIR *dirp)
{
	dirp->pos = 0;
}

/** Close directory.
 *
 * @param dirp Open directory
 * @return 0 on success. On error returns -1 and sets errno.
 */
int closedir(DIR *dirp)
{
	errno_t rc = vfs_put(dirp->fd);
	free(dirp);

	if (rc == EOK) {
		return 0;
	} else {
		errno = rc;
		return -1;
	}
}

/** @}
 */
