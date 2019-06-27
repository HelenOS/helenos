/*
 * Copyright (c) 2016 Jakub Jermar
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

/** @addtogroup vfs
 * @{
 */

/**
 * @file vfs_pager.c
 * @brief VFS pager operations.
 */

#include "vfs.h"
#include <async.h>
#include <fibril_synch.h>
#include <errno.h>
#include <as.h>

void vfs_page_in(ipc_call_t *req)
{
	aoff64_t offset = ipc_get_arg1(req);
	size_t page_size = ipc_get_arg2(req);
	int fd = ipc_get_arg3(req);
	void *page;
	errno_t rc;

	page = as_area_create(AS_AREA_ANY, page_size,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE,
	    AS_AREA_UNPAGED);

	if (page == AS_MAP_FAILED) {
		async_answer_0(req, ENOMEM);
		return;
	}

	rdwr_io_chunk_t chunk = {
		.buffer = page,
		.size = page_size
	};

	size_t total = 0;
	aoff64_t pos = offset;
	do {
		rc = vfs_rdwr_internal(fd, pos, true, &chunk);
		if (rc != EOK)
			break;
		if (chunk.size == 0)
			break;
		total += chunk.size;
		pos += chunk.size;
		chunk.buffer += chunk.size;
		chunk.size = page_size - total;
	} while (total < page_size);

	async_answer_1(req, rc, (sysarg_t) page);

	/*
	 * FIXME:
	 * This is just for now until we implement proper page cache
	 * management.  Not keeping the pages around in a cache results in
	 * inherently non-coherent private mappings.
	 */
	as_area_destroy(page);
}

/**
 * @}
 */
