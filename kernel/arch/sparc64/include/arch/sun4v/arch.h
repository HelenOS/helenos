/*
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief	Various sun4v-specific macros.
 */

#ifndef KERN_sparc64_sun4v_ARCH_H_
#define KERN_sparc64_sun4v_ARCH_H_

/* scratch pad registers ASI */
#define	ASI_SCRATCHPAD		0x20

/*
 * Assignment of scratchpad register virtual addresses. The same convention is
 * used by both Linux and Solaris.
 */

/* register where the address of the MMU fault status area will be stored */
#define SCRATCHPAD_MMU_FSA	0x00

/* register where the CPUID will be stored */
#define SCRATCHPAD_CPUID	0x08

/* register where the kernel stack address will be stored */
#define SCRATCHPAD_KSTACK	0x10

/* register where the userspace window buffer address will be stored */
#define SCRATCHPAD_WBUF		0x18

#endif

/** @}
 */
