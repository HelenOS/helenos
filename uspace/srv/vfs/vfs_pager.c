/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
