/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/**
 * @file
 * @brief This file contains level N interrupt and inter-processor interrupt
 * trap handler.
 */
#ifndef KERN_sparc64_INTERRUPT_TRAP_H_
#define KERN_sparc64_INTERRUPT_TRAP_H_

#define TT_INTERRUPT_LEVEL_1			0x41
#define TT_INTERRUPT_LEVEL_2			0x42
#define TT_INTERRUPT_LEVEL_3			0x43
#define TT_INTERRUPT_LEVEL_4			0x44
#define TT_INTERRUPT_LEVEL_5			0x45
#define TT_INTERRUPT_LEVEL_6			0x46
#define TT_INTERRUPT_LEVEL_7			0x47
#define TT_INTERRUPT_LEVEL_8			0x48
#define TT_INTERRUPT_LEVEL_9			0x49
#define TT_INTERRUPT_LEVEL_10			0x4a
#define TT_INTERRUPT_LEVEL_11			0x4b
#define TT_INTERRUPT_LEVEL_12			0x4c
#define TT_INTERRUPT_LEVEL_13			0x4d
#define TT_INTERRUPT_LEVEL_14			0x4e
#define TT_INTERRUPT_LEVEL_15			0x4f

#define INTERRUPT_LEVEL_N_HANDLER_SIZE		TRAP_TABLE_ENTRY_SIZE

/* IMAP register bits */
#define IGN_MASK	0x7c0
#define INO_MASK	0x1f
#define IMAP_V_MASK	(1ULL << 31)

#define IGN_SHIFT	6

#ifndef __ASSEMBLER__

#include <arch/interrupt.h>

extern void interrupt(unsigned int n, istate_t *istate);

#endif /* !def __ASSEMBLER__ */

#if defined (SUN4U)
#include <arch/trap/sun4u/interrupt.h>
#elif defined (SUN4V)
#include <arch/trap/sun4v/interrupt.h>
#endif

#endif

/** @}
 */
