/*
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CPU_FAMILY_H_
#define KERN_sparc64_CPU_FAMILY_H_

#include <arch.h>
#include <cpu.h>
#include <arch/register.h>
#include <arch/asm.h>

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US family
 */
static inline bool is_us(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCI) || (impl == IMPL_ULTRASPARCII) ||
	    (impl == IMPL_ULTRASPARCII_I) ||  (impl == IMPL_ULTRASPARCII_E);
}

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US-III subfamily
 */
static inline bool is_us_iii(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCIII) ||
	    (impl == IMPL_ULTRASPARCIII_PLUS) ||
	    (impl == IMPL_ULTRASPARCIII_I);
}

/**
 * Find the processor (sub)family.
 *
 * @return 	true iff the CPU belongs to the US-IV subfamily
 */
static inline bool is_us_iv(void)
{
	int impl = ((ver_reg_t) ver_read()).impl;
	return (impl == IMPL_ULTRASPARCIV) || (impl == IMPL_ULTRASPARCIV_PLUS);
}

#endif

/** @}
 */
