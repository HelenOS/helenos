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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief	Backend for address space areas backed by the user pager.
 *
 */

#include <mm/as.h>
#include <mm/page.h>
#include <abi/ipc/methods.h>
#include <ipc/sysipc.h>
#include <synch/mutex.h>
#include <typedefs.h>
#include <align.h>
#include <debug.h>
#include <errno.h>
#include <log.h>

static bool user_create(as_area_t *);
static bool user_resize(as_area_t *, size_t);
static void user_share(as_area_t *);
static void user_destroy(as_area_t *);

static bool user_is_resizable(as_area_t *);
static bool user_is_shareable(as_area_t *);

static int user_page_fault(as_area_t *, uintptr_t, pf_access_t);
static void user_frame_free(as_area_t *, uintptr_t, uintptr_t);

mem_backend_t user_backend = {
	.create = user_create,
	.resize = user_resize,
	.share = user_share,
	.destroy = user_destroy,

	.is_resizable = user_is_resizable,
	.is_shareable = user_is_shareable,

	.page_fault = user_page_fault,
	.frame_free = user_frame_free,

	.create_shared_data = NULL,
	.destroy_shared_data = NULL
};

bool user_create(as_area_t *area)
{
	return true;
}

bool user_resize(as_area_t *area, size_t new_pages)
{
	return true;
}

/** Share the user-paged address space area.
 *
 * The address space and address space area must be already locked.
 *
 * @param area Address space area to be shared.
 */
void user_share(as_area_t *area)
{
	ASSERT(mutex_locked(&area->as->lock));
	ASSERT(mutex_locked(&area->lock));
}

void user_destroy(as_area_t *area)
{
	return;
}

bool user_is_resizable(as_area_t *area)
{
	return true;
}

bool user_is_shareable(as_area_t *area)
{
	return true;
}

/** Service a page fault in the user-paged address space area.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Pointer to the address space area.
 * @param upage Faulting virtual page.
 * @param access Access mode that caused the fault (i.e. read/write/exec).
 *
 * @return AS_PF_FAULT on failure (i.e. page fault) or AS_PF_OK on success (i.e.
 *     serviced).
 */
int user_page_fault(as_area_t *area, uintptr_t upage, pf_access_t access)
{
	ASSERT(page_table_locked(AS));
	ASSERT(mutex_locked(&area->lock));
	ASSERT(IS_ALIGNED(upage, PAGE_SIZE));

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	ipc_data_t data = {};
	IPC_SET_IMETHOD(data, IPC_M_PAGE_IN);
	IPC_SET_ARG1(data, upage - area->base);
	IPC_SET_ARG2(data, PAGE_SIZE);

	int rc = ipc_req_internal(area->backend_data.pager, &data);

	if (rc != EOK) {
		log(LF_USPACE, LVL_FATAL,
		    "Page-in request for page %#" PRIxn
		    " at pager %d failed with error %d.",
		    upage, area->backend_data.pager, rc);
		return AS_PF_FAULT;
	}

	if (IPC_GET_RETVAL(data) != EOK)
		return AS_PF_FAULT;

	/*
	 * A successful reply will contain the physical frame in ARG1.
	 * The physical frame will have the reference count already
	 * incremented (if applicable).
	 */

	uintptr_t frame = IPC_GET_ARG1(data);
	page_mapping_insert(AS, upage, frame, as_area_get_flags(area));
	if (!used_space_insert(area, upage, 1))
		panic("Cannot insert used space.");

	return AS_PF_OK;
}

/** Free a frame that is backed by the user memory backend.
 *
 * The address space area and page tables must be already locked.
 *
 * @param area Ignored.
 * @param page Virtual address of the page corresponding to the frame.
 * @param frame Frame to be released.
 */
void user_frame_free(as_area_t *area, uintptr_t page, uintptr_t frame)
{
	ASSERT(page_table_locked(area->as));
	ASSERT(mutex_locked(&area->lock));
}

/** @}
 */
