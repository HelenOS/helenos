/*
 * Copyright (c) 2006 Jakub Jermar
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
