/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_ISTATE_H_
#define KERN_amd64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL
#include <arch/istate_struct.h>
#else
#include <libarch/istate_struct.h>
#endif

#define RPL_USER	3

/** Return true if exception happened while in userspace */
_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return (istate->cs & RPL_USER) == RPL_USER;
}

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->rip = retaddr;
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->rip;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	return istate->rbp;
}

#endif

/** @}
 */
