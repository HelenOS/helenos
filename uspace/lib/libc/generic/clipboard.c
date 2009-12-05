/*
 * Copyright (c) 2009 Jiri Svoboda
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
 * @brief System clipboard API.
 *
 * The clipboard data is stored in a file and it is shared by the entire
 * system.
 *
 * @note Synchronization is missing. File locks would be ideal for the job.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <macros.h>
#include <errno.h>
#include <clipboard.h>

/** File used to store clipboard data */
#define CLIPBOARD_FILE "/data/clip"

#define CHUNK_SIZE	4096

/** Copy string to clipboard.
 *
 * Sets the clipboard contents to @a str. Passing an empty string or NULL
 * makes the clipboard empty. 
 *
 * @param str	String to put to clipboard or NULL.
 * @return	Zero on success or negative error code.
 */
int clipboard_put_str(const char *str)
{
	int fd;
	const char *sp;
	ssize_t to_write, written, left;

	fd = open(CLIPBOARD_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0666);
	if (fd < 0)
		return EIO;

	left = str_size(str);
	sp = str;

	while (left > 0) {
		to_write = min(left, CHUNK_SIZE);
		written = write(fd, sp, to_write);

		if (written < 0) {
			close(fd);
			unlink(CLIPBOARD_FILE);
			return EIO;
		}

		sp += written;
		left -= written;
	}

	if (close(fd) != EOK) {
		unlink(CLIPBOARD_FILE);
		return EIO;
	}

	return EOK;
}

/** Get a copy of clipboard contents.
 *
 * Returns a new string that can be deallocated with free().
 *
 * @param str	Here pointer to the newly allocated string is stored.
 * @return	Zero on success or negative error code.
 */
int clipboard_get_str(char **str)
{
	int fd;
	char *sbuf, *sp;
	ssize_t to_read, n_read, left;
	struct stat cbs;

	if (stat(CLIPBOARD_FILE, &cbs) != EOK)
		return EIO;

	sbuf = malloc(cbs.size + 1);
	if (sbuf == NULL)
		return ENOMEM;

	fd = open(CLIPBOARD_FILE, O_RDONLY);
	if (fd < 0) {
		free(sbuf);
		return EIO;
	}

	sp = sbuf;
	left = cbs.size;

	while (left > 0) {
		to_read = min(left, CHUNK_SIZE);
		n_read = read(fd, sp, to_read);

		if (n_read < 0) {
			close(fd);
			free(sbuf);
			return EIO;
		}

		sp += n_read;
		left -= n_read;
	}

	close(fd);

	*sp = '\0';
	*str = sbuf;

	return EOK;
}

/** @}
 */
