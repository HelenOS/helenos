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
/** @file Create new directory.
 */

#include <errno.h>
#include <stdio.h>
#include <str.h>
#include <vfs/vfs.h>

#include "fmgt.h"
#include "../private/fmgt.h"
#include "../private/fsops.h"

#define NEWNAME_LEN 64

/** Suggest name for new directory.
 *
 * @param fmgt File management object
 * @param rstr Place to store pointer to newly allocated string
 * @return EOK on success or an error code
 */
errno_t fmgt_new_dir_suggest(char **rstr)
{
	errno_t rc;
	unsigned u;
	vfs_stat_t stat;
	char name[NEWNAME_LEN];

	u = 0;
	while (true) {
		snprintf(name, sizeof(name), "dir%02u", u);
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

/** Create new directory.
 *
 * @param fmgt File management object
 * @param dname Directory name
 * @return EOK on success or an error code
 */
errno_t fmgt_new_dir(fmgt_t *fmgt, const char *dname)
{
	errno_t rc;

	/* Clear statistics. */
	fmgt_progress_init(fmgt);
	fmgt_initial_progress_update(fmgt);

	rc = fmgt_create_dir(fmgt, dname, true);

	fmgt_final_progress_update(fmgt);
	return rc;
}

/** @}
 */
