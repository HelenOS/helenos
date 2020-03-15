/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#include <synch/spinlock.h>
#include <ipc/ipc.h>
#include <arch.h>
#include <proc/task.h>
#include <ipc/ipcrsc.h>
#include <assert.h>
#include <abi/errno.h>
#include <cap/cap.h>
#include <mm/slab.h>
#include <stdlib.h>

static void phone_destroy(void *arg)
{
	phone_t *phone = (phone_t *) arg;
	if (phone->hangup_call)
		kobject_put(phone->hangup_call->kobject);
	slab_free(phone_cache, phone);
}

kobject_ops_t phone_kobject_ops = {
	.destroy = phone_destroy
};

/** Allocate new phone in the specified task.
 *
 * @param[in]  task     Task for which to allocate a new phone.
 * @param[in]  publish  If true, the new capability will be published.
 * @param[out] phandle  New phone capability handle.
 * @param[out] kobject  New phone kobject.
 *
 * @return  An error code if a new capability cannot be allocated.
 */
errno_t phone_alloc(task_t *task, bool publish, cap_phone_handle_t *phandle,
    kobject_t **kobject)
{
	cap_handle_t handle;
	errno_t rc = cap_alloc(task, &handle);
	if (rc == EOK) {
		phone_t *phone = slab_alloc(phone_cache, FRAME_ATOMIC);
		if (!phone) {
			cap_free(TASK, handle);
			return ENOMEM;
		}
		kobject_t *kobj = kobject_alloc(FRAME_ATOMIC);
		if (!kobj) {
			cap_free(TASK, handle);
			slab_free(phone_cache, phone);
			return ENOMEM;
		}
		call_t *hcall = ipc_call_alloc();
		if (!hcall) {
			cap_free(TASK, handle);
			slab_free(phone_cache, phone);
			free(kobj);
			return ENOMEM;
		}

		ipc_phone_init(phone, task);
		phone->state = IPC_PHONE_CONNECTING;
		phone->hangup_call = hcall;

		kobject_initialize(kobj, KOBJECT_TYPE_PHONE, phone);
		phone->kobject = kobj;

		if (publish)
			cap_publish(task, handle, kobj);

		*phandle = handle;
		if (kobject)
			*kobject = kobj;
	}
	return rc;
}

/** Free slot from a disconnected phone.
 *
 * All already sent messages will be correctly processed.
 *
 * @param handle Phone capability handle of the phone to be freed.
 *
 */
void phone_dealloc(cap_phone_handle_t handle)
{
	kobject_t *kobj = cap_unpublish(TASK, handle, KOBJECT_TYPE_PHONE);
	if (!kobj)
		return;

	kobject_put(kobj);
	cap_free(TASK, handle);
}

/** @}
 */
