/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file C standard file manipulation functions
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>
#include <tmpfile.h>
#include <vfs/vfs.h>

/** Static buffer for tmpnam function */
static char tmpnam_buf[L_tmpnam];

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
		/*
		 * Note that ISO C leaves the value of errno undefined,
		 * whereas according to UN*X standards, it is set.
		 */
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
		/*
		 * Note that ISO C leaves the value of errno undefined,
		 * whereas according to UN*X standards, it is set.
		 */
		errno = rc;
		return -1;
	}

	return 0;
}

/** Create a temporary file.
 *
 * @return Open stream descriptor or @c NULL on error. In that case
 *         errno is set (UN*X). Note that ISO C leaves the value of errno
 *         undefined.
 */
FILE *tmpfile(void)
{
	int file;
	FILE *stream;

	file = __tmpfile();
	if (file < 0) {
		printf("file is < 0\n");
		errno = EEXIST;
		return NULL;
	}

	stream = fdopen(file, "w+");
	if (stream == NULL) {
		printf("stream = NULL\n");
		vfs_put(file);
		errno = EACCES;
		return NULL;
	}

	return stream;
}

/** Create name for a temporary file.
 *
 * @param s Pointer to array of at least L_tmpnam bytes or @c NULL.
 * @return The pointer @a s or pointer to internal static buffer on success,
 *         @c NULL on error.
 */
char *tmpnam(char *s)
{
	char *p;

	p = (s != NULL) ? s : tmpnam_buf;
	return __tmpnam(p);
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
