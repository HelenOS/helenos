/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_EXCEPTION_H_
#define KERN_ppc32_EXCEPTION_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/istate_struct.h>
#include <arch/msr.h>

#else /* KERNEL */

#include <libarch/istate_struct.h>
#include <libarch/msr.h>

#endif /* KERNEL */

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->pc = retaddr;
}

/** Return true if exception happened while in userspace
 *
 * The contexts of MSR register was stored in SRR1.
 *
 */
_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return (istate->srr1 & MSR_PR) != 0;
}

_NO_TRACE static inline sysarg_t istate_get_pc(istate_t *istate)
{
	return istate->pc;
}

_NO_TRACE static inline sysarg_t istate_get_fp(istate_t *istate)
{
	return istate->sp;
}

#endif

/** @}
 */
