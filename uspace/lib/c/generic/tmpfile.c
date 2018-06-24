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
/** @file Temporary files
 */

#include <errno.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <str.h>
#include <tmpfile.h>
#include <vfs/vfs.h>

static size_t tmpfile_cnt = 0;
static FIBRIL_MUTEX_INITIALIZE(tmpfile_lock);

/** Create and open file suitable as temporary file based on template.
 *
 * This is designed to allow creating temporary files compatible with POSIX
 * mk(s)temp and tempnam, as well as for the use of ISO C tmpfile, tmpnam.
 *
 * @param templ
 * @param create If @c false, only construct file name
 *
 * @return If @a create is true, open file descriptor on success (and
 *         @a *templ is filled in with actual file name),
 *         if @a create is false, zero on success. -1 on failure.
 */
int __tmpfile_templ(char *templ, bool create)
{
	size_t tsize;
	char *p;
	int file;
	errno_t rc;

	tsize = str_size(templ);
	if (tsize < 6)
		return -1;

	p = templ + tsize - 6;
	if (str_cmp(p, "XXXXXX") != 0)
		return -1;

	fibril_mutex_lock(&tmpfile_lock);

	while (tmpfile_cnt < 1000000) {
		snprintf(p, 6 + 1, "%06zu", tmpfile_cnt);
		if (create) {
			/* Try creating file */
			rc = vfs_lookup(templ, WALK_MUST_CREATE |
			    WALK_REGULAR, &file);
			if (rc == EOK) {
				rc = vfs_open(file, MODE_READ | MODE_WRITE);
				if (rc == EOK) {
					++tmpfile_cnt;
					fibril_mutex_unlock(&tmpfile_lock);
					return file;
				}

				vfs_put(file);
			}
		} else {
			/* Test if file exists */
			rc = vfs_lookup(templ, 0, &file);
			if (rc != EOK) {
				++tmpfile_cnt;
				fibril_mutex_unlock(&tmpfile_lock);
				return 0;
			}

			vfs_put(file);
		}

		++tmpfile_cnt;
	}

	fibril_mutex_unlock(&tmpfile_lock);
	return -1;
}

/** Create and open temporary (unnamed) file.
 *
 * @return Open file descriptor on success, -1 on failure.
 */
int __tmpfile(void)
{
	int file;
	char namebuf[L_tmpnam];

	str_cpy(namebuf, L_tmpnam, "/tmp/tmp.XXXXXX");

	file = __tmpfile_templ(namebuf, true);
	if (file < 0)
		return -1;

	(void) vfs_unlink_path(namebuf);
	return file;
}

/** Construct temporary file name.
 *
 * @param namebuf Buffer that can hold at least L_tmpnam bytes
 * @return @a namebuf on success, @c NULL on failure
 */
char *__tmpnam(char *namebuf)
{
	int rc;

	str_cpy(namebuf, L_tmpnam, "/tmp/tmp.XXXXXX");

	rc = __tmpfile_templ(namebuf, false);
	if (rc < 0)
		return NULL;

	return namebuf;
}

/** @}
 */
