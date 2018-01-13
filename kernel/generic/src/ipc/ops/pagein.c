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

/** @addtogroup genericipc
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

	if (!IPC_GET_RETVAL(answer->data)) {

		pte_t pte;
		uintptr_t frame;

		page_table_lock(AS, true);
		bool found = page_mapping_find(AS, IPC_GET_ARG1(answer->data),
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
			IPC_SET_ARG1(answer->data, frame);
		} else {
			IPC_SET_RETVAL(answer->data, ENOENT);
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
