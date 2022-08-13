/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief	Backend for address space areas backed by the user pager.
 *
 */

#include <mm/as.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <abi/mm/as.h>
#include <abi/ipc/methods.h>
#include <ipc/sysipc.h>
#include <synch/mutex.h>
#include <typedefs.h>
#include <align.h>
#include <assert.h>
#include <errno.h>
#include <log.h>
#include <str.h>

static bool user_create(as_area_t *);
static void user_destroy(as_area_t *);

static bool user_is_resizable(as_area_t *);
static bool user_is_shareable(as_area_t *);

static int user_page_fault(as_area_t *, uintptr_t, pf_access_t);
static void user_frame_free(as_area_t *, uintptr_t, uintptr_t);

mem_backend_t user_backend = {
	.create = user_create,
	.resize = NULL,
	.share = NULL,
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

void user_destroy(as_area_t *area)
{
	return;
}

bool user_is_resizable(as_area_t *area)
{
	return false;
}

bool user_is_shareable(as_area_t *area)
{
	return false;
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
	assert(page_table_locked(AS));
	assert(mutex_locked(&area->lock));
	assert(IS_ALIGNED(upage, PAGE_SIZE));

	if (!as_area_check_access(area, access))
		return AS_PF_FAULT;

	as_area_pager_info_t *pager_info = &area->backend_data.pager_info;

	ipc_data_t data = { };
	ipc_set_imethod(&data, IPC_M_PAGE_IN);
	ipc_set_arg1(&data, upage - area->base);
	ipc_set_arg2(&data, PAGE_SIZE);
	ipc_set_arg3(&data, pager_info->id1);
	ipc_set_arg4(&data, pager_info->id2);
	ipc_set_arg5(&data, pager_info->id3);

	errno_t rc = ipc_req_internal(pager_info->pager, &data, (sysarg_t) true);

	if (rc != EOK) {
		log(LF_USPACE, LVL_FATAL,
		    "Page-in request for page %#" PRIxPTR
		    " at pager %p failed with error %s.",
		    upage, pager_info->pager, str_error_name(rc));
		return AS_PF_FAULT;
	}

	if (ipc_get_retval(&data) != EOK)
		return AS_PF_FAULT;

	/*
	 * A successful reply will contain the physical frame in ARG1.
	 * The physical frame will have the reference count already
	 * incremented (if applicable).
	 */

	uintptr_t frame = ipc_get_arg1(&data);
	page_mapping_insert(AS, upage, frame, as_area_get_flags(area));
	if (!used_space_insert(&area->used_space, upage, 1))
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
	assert(page_table_locked(area->as));
	assert(mutex_locked(&area->lock));

	pfn_t pfn = ADDR2PFN(frame);
	if (find_zone(pfn, 1, 0) != (size_t) -1) {
		frame_free(frame, 1);
	} else {
		/* Nothing to do */
	}

}

/** @}
 */
