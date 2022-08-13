/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#include <ipc/sysipc_ops.h>
#include <ipc/ipc.h>
#include <mm/as.h>
#include <mm/page.h>
#include <genarch/mm/page_pt.h>
#include <genarch/mm/page_ht.h>
#include <mm/frame.h>
#include <proc/task.h>
#include <abi/errno.h>
#include <arch.h>

static errno_t pagein_request_preprocess(call_t *call, phone_t *phone)
{
	/*
	 * Allow only requests from numerically higher task IDs to
	 * numerically lower task IDs to prevent deadlock in
	 * pagein_answer_preprocess() that could happen if two tasks
	 * wanted to be each other's pager.
	 */
	if (TASK->taskid <= phone->callee->task->taskid)
		return ENOTSUP;
	else
		return EOK;
}

static errno_t pagein_answer_preprocess(call_t *answer, ipc_data_t *olddata)
{
	/*
	 * We only do the special handling below if the call was initiated by
	 * the kernel. Otherwise a malicious task could use this mechanism to
	 * hold memory frames forever.
	 */
	if (!answer->priv)
		return EOK;

	if (!ipc_get_retval(&answer->data)) {

		pte_t pte;
		uintptr_t frame;

		page_table_lock(AS, true);
		bool found = page_mapping_find(AS, ipc_get_arg1(&answer->data),
		    false, &pte);
		if (found & PTE_PRESENT(&pte)) {
			frame = PTE_GET_FRAME(&pte);
			pfn_t pfn = ADDR2PFN(frame);
			if (find_zone(pfn, 1, 0) != (size_t) -1) {
				/*
				 * The frame is in physical memory managed by
				 * the frame allocator.
				 */
				frame_reference_add(ADDR2PFN(frame));
			}
			ipc_set_arg1(&answer->data, frame);
		} else {
			ipc_set_retval(&answer->data, ENOENT);
		}
		page_table_unlock(AS, true);
	}

	return EOK;
}

sysipc_ops_t ipc_m_page_in_ops = {
	.request_preprocess = pagein_request_preprocess,
	.request_forget = null_request_forget,
	.request_process = null_request_process,
	.answer_cleanup = null_answer_cleanup,
	.answer_preprocess = pagein_answer_preprocess,
	.answer_process = null_answer_process,
};

/** @}
 */
