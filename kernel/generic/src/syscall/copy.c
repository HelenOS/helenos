/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief	Copying between kernel and userspace.
 *
 * This file contains sanitized functions for copying data
 * between kernel and userspace.
 */

#include <assert.h>
#include <syscall/copy.h>
#include <proc/thread.h>
#include <mm/as.h>
#include <macros.h>
#include <arch.h>
#include <errno.h>

/** Copy data from userspace to kernel.
 *
 * Provisions are made to return value even after page fault.
 *
 * This function can be called only from syscall.
 *
 * @param dst Destination kernel address.
 * @param uspace_src Source userspace address.
 * @param size Size of the data to be copied.
 *
 * @return EOK on success or an error code from @ref errno.h.
 */
errno_t copy_from_uspace(void *dst, uspace_addr_t uspace_src, size_t size)
{
	ipl_t ipl;
	errno_t rc;

	assert(THREAD);
	assert(!THREAD->in_copy_from_uspace);

	if (!KERNEL_ADDRESS_SPACE_SHADOWED) {
		if (overlaps(uspace_src, size,
		    KERNEL_ADDRESS_SPACE_START,
		    KERNEL_ADDRESS_SPACE_END - KERNEL_ADDRESS_SPACE_START)) {
			/*
			 * The userspace source block conflicts with kernel address space.
			 */
			return EPERM;
		}
	}

#ifdef ADDRESS_SPACE_HOLE_START
	/*
	 * Check whether the address is outside the address space hole.
	 */
	if (overlaps(uspace_src, size, ADDRESS_SPACE_HOLE_START,
	    ADDRESS_SPACE_HOLE_END - ADDRESS_SPACE_HOLE_START))
		return EPERM;
#endif

	ipl = interrupts_disable();
	THREAD->in_copy_from_uspace = true;

	rc = !memcpy_from_uspace(dst, uspace_src, size) ? EPERM : EOK;

	THREAD->in_copy_from_uspace = false;

	interrupts_restore(ipl);
	return rc;
}

/** Copy data from kernel to userspace.
 *
 * Provisions are made to return value even after page fault.
 *
 * This function can be called only from syscall.
 *
 * @param uspace_dst Destination userspace address.
 * @param src Source kernel address.
 * @param size Size of the data to be copied.
 *
 * @return 0 on success or an error code from @ref errno.h.
 */
errno_t copy_to_uspace(uspace_addr_t uspace_dst, const void *src, size_t size)
{
	ipl_t ipl;
	errno_t rc;

	assert(THREAD);
	assert(!THREAD->in_copy_to_uspace);

	if (!KERNEL_ADDRESS_SPACE_SHADOWED) {
		if (overlaps(uspace_dst, size,
		    KERNEL_ADDRESS_SPACE_START,
		    KERNEL_ADDRESS_SPACE_END - KERNEL_ADDRESS_SPACE_START)) {
			/*
			 * The userspace destination block conflicts with kernel address space.
			 */
			return EPERM;
		}
	}

#ifdef ADDRESS_SPACE_HOLE_START
	/*
	 * Check whether the address is outside the address space hole.
	 */
	if (overlaps(uspace_dst, size, ADDRESS_SPACE_HOLE_START,
	    ADDRESS_SPACE_HOLE_END - ADDRESS_SPACE_HOLE_START))
		return EPERM;
#endif

	ipl = interrupts_disable();
	THREAD->in_copy_to_uspace = true;

	rc = !memcpy_to_uspace(uspace_dst, src, size) ? EPERM : EOK;

	THREAD->in_copy_to_uspace = false;

	interrupts_restore(ipl);
	return rc;
}

/** @}
 */
