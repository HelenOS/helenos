/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_proc
 * @{
 */
/** @file
 */

#include <proc/thread.h>
#include <arch/proc/thread.h>
#include <mm/slab.h>
#include <arch/trap/regwin.h>
#include <align.h>

slab_cache_t *uwb_cache = NULL;

void thr_constructor_arch(thread_t *t)
{
	/*
	 * Allocate memory for uspace_window_buffer.
	 */
	t->arch.uspace_window_buffer = NULL;
}

void thr_destructor_arch(thread_t *t)
{
	if (t->arch.uspace_window_buffer) {
		uintptr_t uw_buf = (uintptr_t) t->arch.uspace_window_buffer;
		/*
		 * Mind the possible alignment of the userspace window buffer
		 * belonging to a killed thread.
		 */
		slab_free(uwb_cache, (uint8_t *) ALIGN_DOWN(uw_buf,
		    UWB_ALIGNMENT));
	}
}

errno_t thread_create_arch(thread_t *t, thread_flags_t flags)
{
	if ((flags & THREAD_FLAG_USPACE) && (!t->arch.uspace_window_buffer)) {
		/*
		 * The thread needs userspace window buffer and the object
		 * returned from the slab allocator doesn't have any.
		 */
		t->arch.uspace_window_buffer = slab_alloc(uwb_cache, FRAME_ATOMIC);
		if (!t->arch.uspace_window_buffer)
			return ENOMEM;
	} else {
		uintptr_t uw_buf = (uintptr_t) t->arch.uspace_window_buffer;

		/*
		 * Mind the possible alignment of the userspace window buffer
		 * belonging to a killed thread.
		 */
		t->arch.uspace_window_buffer = (uint8_t *) ALIGN_DOWN(uw_buf,
		    UWB_ALIGNMENT);
	}
	return EOK;
}

/** @}
 */
