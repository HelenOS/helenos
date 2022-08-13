/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file File status handling.
 */

#include "../internal/common.h"
#include <sys/stat.h>
#include <vfs/vfs.h>

#include <errno.h>
#include <mem.h>

/**
 * Convert HelenOS stat struct into POSIX stat struct (if possible).
 *
 * @param dest POSIX stat struct.
 * @param src HelenOS stat struct.
 *
 * @return 0 on success, -1 on error.
 */
static int stat_to_posix(struct stat *dest, vfs_stat_t *src)
{
	memset(dest, 0, sizeof(struct stat));

	dest->st_dev = src->service;
	dest->st_ino = src->index;

	/* HelenOS doesn't support permissions, so we set them all */
	dest->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	if (src->is_file) {
		dest->st_mode |= S_IFREG;
	}
	if (src->is_directory) {
		dest->st_mode |= S_IFDIR;
	}

	dest->st_nlink = src->lnkcnt;
	dest->st_size = src->size;

	if (src->size > INT64_MAX) {
		errno = ERANGE;
		return -1;
	}

	return 0;
}

/**
 * Retrieve file status for file associated with file descriptor.
 *
 * @param fd File descriptor of the opened file.
 * @param st Status structure to be filled with information.
 * @return Zero on success, -1 otherwise.
 */
int fstat(int fd, struct stat *st)
{
	vfs_stat_t hst;
	if (failed(vfs_stat(fd, &hst)))
		return -1;
	return stat_to_posix(st, &hst);
}

/**
 * Retrieve file status for symbolic link.
 *
 * @param path Path to the symbolic link.
 * @param st Status structure to be filled with information.
 * @return Zero on success, -1 otherwise.
 */
int lstat(const char *restrict path, struct stat *restrict st)
{
	/* There are currently no symbolic links in HelenOS. */
	return stat(path, st);
}

/**
 * Retrieve file status for regular file (or symbolic link target).
 *
 * @param path Path to the file/link.
 * @param st Status structure to be filled with information.
 * @return Zero on success, -1 otherwise.
 */
int stat(const char *restrict path, struct stat *restrict st)
{
	vfs_stat_t hst;
	if (failed(vfs_stat_path(path, &hst)))
		return -1;
	return stat_to_posix(st, &hst);
}

/**
 * Change permission bits for the file if possible.
 *
 * @param path Path to the file.
 * @param mode Permission bits to be set.
 * @return Zero on success, -1 otherwise.
 */
int chmod(const char *path, mode_t mode)
{
	/* HelenOS doesn't support permissions, return success. */
	return 0;
}

/**
 * Set the file mode creation mask of the process.
 *
 * @param mask Set permission bits are cleared in the related creation
 *     functions. Non-permission bits are ignored.
 * @return Previous file mode creation mask.
 */
mode_t umask(mode_t mask)
{
	/* HelenOS doesn't support permissions, return empty mask. */
	return 0;
}

/**
 * Create a directory.
 *
 * @param path Path to the new directory.
 * @param mode Permission bits to be set.
 * @return Zero on success, -1 otherwise.
 */
int mkdir(const char *path, mode_t mode)
{
	if (failed(vfs_link_path(path, KIND_DIRECTORY, NULL)))
		return -1;
	else
		return 0;
}

/** @}
 */
