/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_interrupt
 * @{
 */

#ifndef KERN_arm64_ISTATE_H_
#define KERN_arm64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/istate_struct.h>
#include <arch/regutils.h>

#else /* KERNEL */

#include <libarch/istate_struct.h>
#include <libarch/regutils.h>

#endif /* KERNEL */

/** Set Program Counter member of given istate structure.
 *
 * @param istate  istate structure
 * @param retaddr new value of istate's PC member
 */
_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->pc = retaddr;
}

/** Return true if exception happened while in userspace. */
_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return (istate->spsr & SPSR_MODE_MASK) >> SPSR_MODE_SHIFT ==
	    SPSR_MODE_ARM64_EL0T;
}

/** Return Program Counter member of given istate structure. */
_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->pc;
}

/** Return Frame Pointer member of given istate structure. */
_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	return istate->x29;
}

#endif

/** @}
 */
