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
/** @file File management library.
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

#define NEWNAME_LEN 64

/** Create file management library instance.
 *
 * @param rfmgt Place to store pointer to new file management library instance
 * @return EOK on succcess, ENOMEM if out of memory.
 */
errno_t fmgt_create(fmgt_t **rfmgt)
{
	fmgt_t *fmgt;

	fmgt = calloc(1, sizeof(fmgt_t));
	if (fmgt == NULL)
		return ENOMEM;

	*rfmgt = fmgt;
	return EOK;
}

/** Create file management library instance.
 *
 * @param fmgt File management object
 * @param cb Callback functions
 * @param arg Argument to callback functions
 * @return EOK on succcess, ENOMEM if out of memory.
 */
void fmgt_set_cb(fmgt_t *fmgt, fmgt_cb_t *cb, void *arg)
{
	fmgt->cb = cb;
	fmgt->cb_arg = arg;
}

/** Destroy file management library instance.
 *
 * @param fmgt File management object
 */
void fmgt_destroy(fmgt_t *fmgt)
{
	free(fmgt);
}

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

/** @}
 */
