/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt sparc64
 * @ingroup interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_ISTATE_H_
#define KERN_sparc64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/istate_struct.h>
#include <arch/regdef.h>

#else /* KERNEL */

#include <libarch/istate_struct.h>
#include <libarch/regdef.h>

#endif /* KERNEL */

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->tpc = retaddr;
}

_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return !(istate->tstate & TSTATE_PRIV_BIT);
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->tpc;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	/* TODO */
	return 0;
}

#endif

/** @}
 */
