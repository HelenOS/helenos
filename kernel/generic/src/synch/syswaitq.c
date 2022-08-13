/*
 * SPDX-FileCopyrightText: 2018 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void waitq_destroy(void *arg)
{
	waitq_t *wq = (waitq_t *) arg;
	slab_free(waitq_cache, wq);
}

kobject_ops_t waitq_kobject_ops = {
	.destroy = waitq_destroy
};

static bool waitq_cap_cleanup_cb(cap_t *cap, void *arg)
{
	kobject_t *kobj = cap_unpublish(cap->task, cap->handle,
	    KOBJECT_TYPE_WAITQ);
	kobject_put(kobj);
	cap_free(cap->task, cap->handle);
	return true;
}

/** Initialize the user waitq subsystem */
void sys_waitq_init(void)
{
	waitq_cache = slab_cache_create("waitq_t", sizeof(waitq_t), 0, NULL,
	    NULL, 0);
}

/** Clean-up all waitq capabilities held by the exiting task */
void sys_waitq_task_cleanup(void)
{
	caps_apply_to_kobject_type(TASK, KOBJECT_TYPE_WAITQ,
	    waitq_cap_cleanup_cb, NULL);
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
	waitq_t *wq = slab_alloc(waitq_cache, FRAME_ATOMIC);
	if (!wq)
		return (sys_errno_t) ENOMEM;
	waitq_initialize(wq);

	kobject_t *kobj = kobject_alloc(0);
	if (!kobj) {
		slab_free(waitq_cache, wq);
		return (sys_errno_t) ENOMEM;
	}
	kobject_initialize(kobj, KOBJECT_TYPE_WAITQ, wq);

	cap_handle_t handle;
	errno_t rc = cap_alloc(TASK, &handle);
	if (rc != EOK) {
		slab_free(waitq_cache, wq);
		kobject_free(kobj);
		return (sys_errno_t) rc;
	}

	rc = copy_to_uspace(whandle, &handle, sizeof(handle));
	if (rc != EOK) {
		cap_free(TASK, handle);
		kobject_free(kobj);
		slab_free(waitq_cache, wq);
		return (sys_errno_t) rc;
	}

	cap_publish(TASK, handle, kobj);

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
	kobject_t *kobj = kobject_get(TASK, whandle, KOBJECT_TYPE_WAITQ);
	if (!kobj)
		return (sys_errno_t) ENOENT;

#ifdef CONFIG_UDEBUG
	udebug_stoppable_begin();
#endif

	errno_t rc = waitq_sleep_timeout(kobj->waitq, timeout,
	    SYNCH_FLAGS_INTERRUPTIBLE | flags, NULL);

#ifdef CONFIG_UDEBUG
	udebug_stoppable_end();
#endif

	kobject_put(kobj);

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
	kobject_t *kobj = kobject_get(TASK, whandle, KOBJECT_TYPE_WAITQ);
	if (!kobj)
		return (sys_errno_t) ENOENT;

	waitq_wakeup(kobj->waitq, WAKEUP_FIRST);

	kobject_put(kobj);
	return (sys_errno_t) EOK;
}

/** @}
 */
