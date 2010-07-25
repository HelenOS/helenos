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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <as.h>
#include <libc.h>
#include <unistd.h>
#include <align.h>
#include <sys/types.h>
#include <bitops.h>
#include <malloc.h>

/** Last position allocated by as_get_mappable_page */
static uintptr_t last_allocated = 0;

/** Create address space area.
 *
 * @param address Virtual address where to place new address space area.
 * @param size    Size of the area.
 * @param flags   Flags describing type of the area.
 *
 * @return address on success, (void *) -1 otherwise.
 *
 */
void *as_area_create(void *address, size_t size, int flags)
{
	return (void *) __SYSCALL3(SYS_AS_AREA_CREATE, (sysarg_t) address,
	    (sysarg_t) size, (sysarg_t) flags);
}

/** Resize address space area.
 *
 * @param address Virtual address pointing into already existing address space
 *                area.
 * @param size    New requested size of the area.
 * @param flags   Currently unused.
 *
 * @return zero on success or a code from @ref errno.h on failure.
 *
 */
int as_area_resize(void *address, size_t size, int flags)
{
	return __SYSCALL3(SYS_AS_AREA_RESIZE, (sysarg_t) address,
	    (sysarg_t) size, (sysarg_t) flags);
}

/** Destroy address space area.
 *
 * @param address Virtual address pointing into the address space area being
 *                destroyed.
 *
 * @return zero on success or a code from @ref errno.h on failure.
 *
 */
int as_area_destroy(void *address)
{
	return __SYSCALL1(SYS_AS_AREA_DESTROY, (sysarg_t) address);
}

/** Change address-space area flags.
 *
 * @param address Virtual address pointing into the address space area being
 *                modified.
 * @param flags   New flags describing type of the area.
 *
 * @return zero on success or a code from @ref errno.h on failure.
 *
 */
int as_area_change_flags(void *address, int flags)
{
	return __SYSCALL2(SYS_AS_AREA_CHANGE_FLAGS, (sysarg_t) address,
	    (sysarg_t) flags);
}

/** Return pointer to some unmapped area, where fits new as_area
 *
 * @param size Requested size of the allocation.
 *
 * @return pointer to the beginning
 *
 */
void *as_get_mappable_page(size_t size)
{
	if (size == 0)
		return NULL;
	
	size_t sz = 1 << (fnzb(size - 1) + 1);
	if (last_allocated == 0)
		last_allocated = get_max_heap_addr();
	
	/*
	 * Make sure we allocate from naturally aligned address.
	 */
	uintptr_t res = ALIGN_UP(last_allocated, sz);
	last_allocated = res + ALIGN_UP(size, PAGE_SIZE);
	
	return ((void *) res);
}

/** @}
 */
