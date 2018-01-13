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
#include <errno.h>
#include <align.h>
#include <stddef.h>
#include <stdint.h>
#include <bitops.h>
#include "private/libc.h"

/** Create address space area.
 *
 * @param base       Starting virtual address of the area.
 *                   If set to AS_AREA_ANY ((void *) -1), the kernel finds a
 *                   mappable area.
 * @param size       Size of the area.
 * @param flags      Flags describing type of the area.
 * @param pager_info Pager info structure or AS_AREA_UNPAGED (NULL) if the area
 *                   is not paged (i.e. anonymous).
 *
 * @return Starting virtual address of the created area on success.
 * @return AS_MAP_FAILED ((void *) -1) otherwise.
 *
 */
void *as_area_create(void *base, size_t size, unsigned int flags,
    as_area_pager_info_t *pager_info)
{
	return (void *) __SYSCALL5(SYS_AS_AREA_CREATE, (sysarg_t) base,
	    (sysarg_t) size, (sysarg_t) flags, (sysarg_t) __entry,
	    (sysarg_t) pager_info);
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
errno_t as_area_resize(void *address, size_t size, unsigned int flags)
{
	return (errno_t) __SYSCALL3(SYS_AS_AREA_RESIZE, (sysarg_t) address,
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
errno_t as_area_destroy(void *address)
{
	return (errno_t) __SYSCALL1(SYS_AS_AREA_DESTROY, (sysarg_t) address);
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
errno_t as_area_change_flags(void *address, unsigned int flags)
{
	return (errno_t) __SYSCALL2(SYS_AS_AREA_CHANGE_FLAGS, (sysarg_t) address,
	    (sysarg_t) flags);
}

/** Find mapping to physical address.
 *
 * @param      virt Virtual address to find mapping for.
 * @param[out] phys Physical adress.
 *
 * @return EOK on no error.
 * @retval ENOENT if no mapping was found.
 *
 */
errno_t as_get_physical_mapping(const void *virt, uintptr_t *phys)
{
	return (errno_t) __SYSCALL2(SYS_PAGE_FIND_MAPPING, (sysarg_t) virt,
	    (sysarg_t) phys);
}

/** @}
 */
