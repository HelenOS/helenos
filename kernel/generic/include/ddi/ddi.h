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

/** @addtogroup genericddi
 * @{
 */
/** @file
 */

#ifndef KERN_DDI_H_
#define KERN_DDI_H_

#include <typedefs.h>
#include <abi/ddi/arg.h>
#include <proc/task.h>
#include <adt/list.h>

/** Structure representing contiguous physical memory area. */
typedef struct {
	link_t link;      /**< Linked list link */
	
	uintptr_t pbase;  /**< Physical base of the area. */
	pfn_t frames;     /**< Number of frames in the area. */
	bool unpriv;      /**< Allow mapping by unprivileged tasks. */
	bool mapped;      /**< Indicate whether the area is actually
	                       mapped. */
} parea_t;

extern void ddi_init(void);
extern void ddi_parea_register(parea_t *);

extern sys_errno_t sys_physmem_map(uintptr_t, size_t, unsigned int, void *,
    uintptr_t);
extern sys_errno_t sys_physmem_unmap(uintptr_t);

extern sys_errno_t sys_dmamem_map(size_t, unsigned int, unsigned int, void *,
    void *, uintptr_t);
extern sys_errno_t sys_dmamem_unmap(uintptr_t, size_t, unsigned int);

extern sys_errno_t sys_iospace_enable(ddi_ioarg_t *);
extern sys_errno_t sys_iospace_disable(ddi_ioarg_t *);

/*
 * Interface to be implemented by all architectures.
 */
extern errno_t ddi_iospace_enable_arch(task_t *, uintptr_t, size_t);
extern errno_t ddi_iospace_disable_arch(task_t *, uintptr_t, size_t);

#endif

/** @}
 */
