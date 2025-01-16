/*
 * Copyright (c) 2018 Jakub Jermar
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

/** @addtogroup kernel_sync
 * @{
 */

/**
 * @file
 * @brief Wrapper for using wait queue as a kobject.
 */

#include <synch/syswaitq.h>
#include <synch/waitq.h>
#include <abi/cap.h>
#include <cap/cap.h>
#include <mm/slab.h>
#include <proc/task.h>
#include <syscall/copy.h>

#include <stdint.h>

static slab_cache_t *waitq_cache;

typedef struct {
	kobject_t kobject;
	waitq_t waitq;
} waitq_kobject_t;

static void waitq_destroy(kobject_t *arg)
{
	waitq_kobject_t *wq = (waitq_kobject_t *) arg;
	slab_free(waitq_cache, wq);
}

kobject_ops_t waitq_kobject_ops = {
	.destroy = waitq_destroy,
};

/** Initialize the user waitq subsystem */
void sys_waitq_init(void)
{
	waitq_cache = slab_cache_create("syswaitq_t", sizeof(waitq_kobject_t),
	    0, NULL, NULL, 0);
}

/** Create a waitq for the current task
 *
 * @param[out] whandle  Userspace address of the destination buffer that will
 *                      receive the allocated waitq capability.
 *
 * @return              Error code.
 */
sys_errno_t sys_waitq_create(uspace_ptr_cap_waitq_handle_t whandle)
{
	waitq_kobject_t *kobj = slab_alloc(waitq_cache, FRAME_ATOMIC);
	if (!kobj)
		return (sys_errno_t) ENOMEM;

	kobject_initialize(&kobj->kobject, KOBJECT_TYPE_WAITQ);
	waitq_initialize(&kobj->waitq);

	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK) {
		slab_free(waitq_cache, kobj);
		return (sys_errno_t) rc;
	}

	rc = copy_to_uspace(whandle, &handle, sizeof(handle));
	if (rc != EOK) {
		cap_free(TASK, handle);
		slab_free(waitq_cache, kobj);
		return (sys_errno_t) rc;
	}

	cap_publish(TASK, handle, &kobj->kobject);

	return (sys_errno_t) EOK;
}

/** Destroy a waitq
 *
 * @param whandle  Waitq capability handle of the waitq to be destroyed.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_destroy(cap_waitq_handle_t whandle)
{
	kobject_t *kobj = cap_unpublish(TASK, whandle, KOBJECT_TYPE_WAITQ);
	if (!kobj)
		return (sys_errno_t) ENOENT;
	kobject_put(kobj);
	cap_free(TASK, whandle);
	return EOK;
}

/** Sleep in the waitq
 *
 * @param whandle  Waitq capability handle of the waitq in which to sleep.
 * @param timeout  Timeout in microseconds.
 * @param flags    Flags from SYNCH_FLAGS_* family. SYNCH_FLAGS_INTERRUPTIBLE is
 *                 always implied.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_sleep(cap_waitq_handle_t whandle, uint32_t timeout,
    unsigned int flags)
{
	waitq_kobject_t *kobj =
	    (waitq_kobject_t *) kobject_get(TASK, whandle, KOBJECT_TYPE_WAITQ);
	if (!kobj)
		return (sys_errno_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif

	errno_t rc = _waitq_sleep_timeout(&kobj->waitq, timeout,
	    SYNCH_FLAGS_INTERRUPTIBLE | flags);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif

	kobject_put(&kobj->kobject);

	return (sys_errno_t) rc;
}

/** Wakeup a thread sleeping in the waitq
 *
 * @param whandle  Waitq capability handle of the waitq to invoke wakeup on.
 *
 * @return         Error code.
 */
sys_errno_t sys_waitq_wakeup(cap_waitq_handle_t whandle)
{
	waitq_kobject_t *kobj =
	    (waitq_kobject_t *) kobject_get(TASK, whandle, KOBJECT_TYPE_WAITQ);
	if (!kobj)
		return (sys_errno_t) ENOENT;

	waitq_wake_one(&kobj->waitq);

	kobject_put(&kobj->kobject);
	return (sys_errno_t) EOK;
}

/** @}
 */
