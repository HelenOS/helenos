/*
 * Copyright (c) 2011 Maurizio Lombardi
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

/** @addtogroup fs
 * @{
 */

#include <libfs.h>
#include <stdlib.h>
#include <libblock.h>
#include <errno.h>
#include <str.h>
#include "mfs.h"
#include "mfs_utils.h"
#include "../../vfs/vfs.h"

static bool check_magic_number(int16_t magic, bool *native, mfs_version_t *version);

void mfs_mounted(ipc_callid_t rid, ipc_call_t *request)
{
	devmap_handle_t devmap_handle = (devmap_handle_t) IPC_GET_ARG1(*request);
	enum cache_mode cmode;	
	struct mfs3_superblock *sp;
	bool native;
	mfs_version_t version;

	/* Accept the mount options */
	char *opts;
	int rc = async_data_write_accept((void **) &opts, true, 0, 0, 0, NULL);
	
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;

	free(opts);

	/* initialize libblock */
	rc = block_init(devmap_handle, MFS_SUPERBLOCK_SIZE);
	if (rc != EOK) {
		async_answer_0(rid, rc);
		return;
	}

	/* prepare the superblock */
	rc = block_bb_read(devmap_handle, MFS_SUPERBLOCK);
	if (rc != EOK) {
		block_fini(devmap_handle);
		async_answer_0(rid, rc);
		return;
	}

	/* get the buffer with the superblock */
	sp = block_bb_get(devmap_handle);

	if (!check_magic_number(sp->s_magic, &native, &version)) {
		/*Magic number is invalid!*/
		block_fini(devmap_handle);
		async_answer_0(rid, ENOTSUP);
		return;
	}
}

static bool check_magic_number(int16_t magic, bool *native, mfs_version_t *version)
{
	if (magic == MFS_MAGIC_V1 || magic == MFS_MAGIC_V1R) {
		*native = magic == MFS_MAGIC_V1;
		*version = MFS_VERSION_V1;
		return true;
	} else if (magic == MFS_MAGIC_V2 || magic == MFS_MAGIC_V2R) {
		*native = magic == MFS_MAGIC_V2;
		*version = MFS_VERSION_V2;
		return true;
	} else if (magic == MFS_MAGIC_V3 || magic == MFS_MAGIC_V3R) {
		*native = magic == MFS_MAGIC_V3;
		*version = MFS_VERSION_V3;
		return true;
	}

	return false;
}


/**
 * @}
 */ 

