/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

/** Open directory.
 *
 * @param dirname Directory pathname
 *
 * @return Non-NULL pointer on success. On error returns @c NULL and sets errno.
 */
DIR *opendir(const char *dirname)
{
	DIR *dirp = malloc(sizeof(DIR));
	if (!dirp) {
		errno = ENOMEM;
		return NULL;
	}

	int fd;
	errno_t rc = vfs_lookup(dirname, WALK_DIRECTORY, &fd);
	if (rc != EOK) {
		free(dirp);
		errno = rc;
		return NULL;
	}

	rc = vfs_open(fd, MODE_READ);
	if (rc != EOK) {
		free(dirp);
		vfs_put(fd);
		errno = rc;
		return NULL;
	}

	dirp->fd = fd;
	dirp->pos = 0;
	return dirp;
}

/** Read directory entry.
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
