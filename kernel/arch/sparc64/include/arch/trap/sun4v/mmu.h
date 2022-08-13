/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains fast MMU trap handlers.
 */

#ifndef KERN_sparc64_sun4v_MMU_TRAP_H_
#define KERN_sparc64_sun4v_MMU_TRAP_H_

#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/arch.h>
#include <arch/sun4v/arch.h>
#include <arch/sun4v/hypercall.h>
#include <arch/mm/sun4v/mmu.h>
#include <arch/mm/tlb.h>
#include <arch/mm/mmu.h>
#include <arch/mm/tte.h>
#include <arch/trap/regwin.h>

#ifdef CONFIG_TSB
#include <arch/mm/tsb.h>
#endif

#define TT_FAST_INSTRUCTION_ACCESS_MMU_MISS	0x64
#define TT_FAST_DATA_ACCESS_MMU_MISS		0x68
#define TT_FAST_DATA_ACCESS_PROTECTION		0x6c
#define TT_CPU_MONDO				0x7c

#define FAST_MMU_HANDLER_SIZE			128

#ifdef __ASSEMBLER__
#include <arch/trap/sun4v/mmu.S>
#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
