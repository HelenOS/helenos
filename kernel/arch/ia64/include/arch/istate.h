/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_ISTATE_H_
#define KERN_ia64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/istate_struct.h>
#include <arch/register.h>

#else /* KERNEL */

#include <libarch/istate_struct.h>
#include <libarch/register.h>

#endif /* KERNEL */

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->cr_iip = retaddr;
	istate->cr_ipsr.ri = 0;    /* return to instruction slot #0 */
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->cr_iip;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	/* FIXME */

	return 0;
}

_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return istate->cr_ipsr.cpl == PSR_CPL_USER;
}

#endif

/** @}
 */
