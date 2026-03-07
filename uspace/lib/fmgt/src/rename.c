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
/** @file Rename file or directory.
 */

#include <errno.h>

#include "fmgt.h"
#include "../private/fmgt.h"
#include "../private/fsops.h"

/** Rename file or directory.
 *
 * @param fmgt File management object
 * @param old_path Original path
 * @param new_name New name (does not need to be a full path)
 * @return EOK on success or an error code
 */
errno_t fmgt_rename(fmgt_t *fmgt, const char *old_path, const char *new_name)
{
	errno_t rc;

	/* Clear statistics. */
	fmgt_progress_init(fmgt);
	fmgt_initial_progress_update(fmgt);

	rc = fmgt_rename_entry(fmgt, old_path, new_name);

	fmgt_final_progress_update(fmgt);
	return rc;
}

/** @}
 */
