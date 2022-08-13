/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_ISTATE_H_
#define KERN_riscv64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL
#include <arch/istate_struct.h>
#else
#include <libarch/istate_struct.h>
#endif

_NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	// FIXME
	return 0;
}

_NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	// FIXME
}

_NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	// FIXME
	return 0;
}

_NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	// FIXME
	return 0;
}

#endif

/** @}
 */
