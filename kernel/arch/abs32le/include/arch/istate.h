/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_ISTATE_H_
#define KERN_abs32le_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <typedefs.h>
#include <verify.h>

#else /* KERNEL */

#include <stdint.h>

#define REQUIRES_EXTENT_MUTABLE(arg)
#define WRITES(arg)

#endif /* KERNEL */

/*
 * On real hardware this stores the registers which
 * need to be preserved during interupts.
 */
typedef struct istate {
	uintptr_t ip;
	uintptr_t fp;
	uint32_t stack[];
} istate_t;

_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/*
	 * On real hardware this checks whether the interrupted
	 * context originated from user space.
	 */

	return !(istate->ip & UINT32_C(0x80000000));
}

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
    WRITES(&istate->ip)
{
	/* On real hardware this sets the instruction pointer. */

	istate->ip = retaddr;
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/* On real hardware this returns the instruction pointer. */

	return istate->ip;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/* On real hardware this returns the frame pointer. */

	return istate->fp;
}

#endif

/** @}
 */
