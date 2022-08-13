/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ISTATE_H_
#define KERN_mips32_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/cp0.h>
#include <arch/istate_struct.h>

#else /* KERNEL */

#include <libarch/cp0.h>
#include <libarch/istate_struct.h>

#endif /* KERNEL */

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->epc = retaddr;
}

/** Return true if exception happened while in userspace */
_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return istate->status & cp0_status_um_bit;
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->epc;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	return istate->sp;
}

#endif

/** @}
 */
