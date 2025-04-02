/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <block.h>
#include <errno.h>
#include <hr.h>
#include <inttypes.h>
#include <stdio.h>
#include <str_error.h>

#include "io.h"
#include "util.h"
#include "var.h"

errno_t hr_io_worker(void *arg)
{
	hr_io_t *io = arg;
	hr_extent_t *extents = (hr_extent_t *)&io->vol->extents;
	size_t ext_idx = io->extent;
	errno_t rc;

	const char *debug_type_str = NULL;
	switch (io->type) {
	case HR_BD_SYNC:
		debug_type_str = "SYNC";
		break;
	case HR_BD_READ:
		debug_type_str = "READ";
		break;
	case HR_BD_WRITE:
		debug_type_str = "WRITE";
		break;
	}

	HR_DEBUG("%s WORKER (%p) on extent: %zu, ba: %" PRIu64 ", "
	    "cnt: %" PRIu64 "\n",
	    debug_type_str, io, io->extent, io->ba, io->cnt);

	switch (io->type) {
	case HR_BD_SYNC:
		rc = block_sync_cache(extents[ext_idx].svc_id,
		    io->ba, io->cnt);
		if (rc == ENOTSUP)
			rc = EOK;
		break;
	case HR_BD_READ:
		rc = block_read_direct(extents[ext_idx].svc_id, io->ba,
		    io->cnt, io->data_read);
		break;
	case HR_BD_WRITE:
		rc = block_write_direct(extents[ext_idx].svc_id, io->ba,
		    io->cnt, io->data_write);
		break;
	default:
		return EINVAL;
	}

	HR_DEBUG("WORKER (%p) rc: %s\n", io, str_error(rc));

	/*
	 * We don't have to invalidate extents who got ENOMEM
	 * on READ/SYNC. But when we get ENOMEM on a WRITE, we have
	 * to invalidate it, because there could have been
	 * other writes, there is no way to rollback.
	 */
	if (rc != EOK && (rc != ENOMEM || io->type == HR_BD_WRITE))
		io->state_callback(io->vol, io->extent, rc);

	return rc;
}

/** @}
 */
