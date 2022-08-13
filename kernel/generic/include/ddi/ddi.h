/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ddi
 * @{
 */
/** @file
 */

#ifndef KERN_DDI_H_
#define KERN_DDI_H_

#include <typedefs.h>
#include <abi/ddi/arg.h>
#include <proc/task.h>
#include <adt/odict.h>

/** Structure representing contiguous physical memory area. */
typedef struct parea {
	/** Link to @c pareas ordered dictionary */
	odlink_t lpareas;

	/** Physical base of the area. */
	uintptr_t pbase;
	/** Number of frames in the area. */
	pfn_t frames;
	/** Allow mapping by unprivileged tasks. */
	bool unpriv;
	/** Indicate whether the area is actually mapped. */
	bool mapped;
	/** Called when @c mapped field has changed */
	void (*mapped_changed)(void *);
	/** Callback argument */
	void *arg;
} parea_t;

extern void ddi_init(void);
extern void ddi_parea_init(parea_t *);
extern void ddi_parea_register(parea_t *);
extern void ddi_parea_unmap_notify(parea_t *);

extern void *pio_map(void *, size_t);
extern void pio_unmap(void *, void *, size_t);

extern sys_errno_t sys_physmem_map(uintptr_t, size_t, unsigned int, uspace_ptr_uintptr_t,
    uintptr_t);
extern sys_errno_t sys_physmem_unmap(uintptr_t);

extern sys_errno_t sys_dmamem_map(size_t, unsigned int, unsigned int, uspace_ptr_uintptr_t,
    uspace_ptr_uintptr_t, uintptr_t);
extern sys_errno_t sys_dmamem_unmap(uintptr_t, size_t, unsigned int);

extern sys_errno_t sys_iospace_enable(uspace_ptr_ddi_ioarg_t);
extern sys_errno_t sys_iospace_disable(uspace_ptr_ddi_ioarg_t);

/*
 * Interface to be implemented by all architectures.
 */
extern errno_t ddi_iospace_enable_arch(task_t *, uintptr_t, size_t);
extern errno_t ddi_iospace_disable_arch(task_t *, uintptr_t, size_t);

#endif

/** @}
 */
