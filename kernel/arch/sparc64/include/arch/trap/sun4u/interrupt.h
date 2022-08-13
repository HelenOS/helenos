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
 * @brief This file contains interrupt vector trap handler.
 */

#ifndef KERN_sparc64_TRAP_SUN4U_INTERRUPT_H_
#define KERN_sparc64_TRAP_SUN4U_INTERRUPT_H_

#include <arch/trap/trap_table.h>
#include <arch/stack.h>

/* Interrupt ASI registers. */
#define ASI_INTR_W			0x77
#define ASI_INTR_DISPATCH_STATUS	0x48
#define ASI_INTR_R			0x7f
#define ASI_INTR_RECEIVE		0x49

/* VA's used with ASI_INTR_W register. */
#if defined (US)
#define ASI_UDB_INTR_W_DATA_0	0x40
#define ASI_UDB_INTR_W_DATA_1	0x50
#define ASI_UDB_INTR_W_DATA_2	0x60
#elif defined (US3)
#define VA_INTR_W_DATA_0	0x40
#define VA_INTR_W_DATA_1	0x48
#define VA_INTR_W_DATA_2	0x50
#define VA_INTR_W_DATA_3	0x58
#define VA_INTR_W_DATA_4	0x60
#define VA_INTR_W_DATA_5	0x68
#define VA_INTR_W_DATA_6	0x80
#define VA_INTR_W_DATA_7	0x88
#endif
#define VA_INTR_W_DISPATCH	0x70

/* VA's used with ASI_INTR_R register. */
#if defined(US)
#define ASI_UDB_INTR_R_DATA_0	0x40
#define ASI_UDB_INTR_R_DATA_1	0x50
#define ASI_UDB_INTR_R_DATA_2	0x60
#elif defined (US3)
#define VA_INTR_R_DATA_0	0x40
#define VA_INTR_R_DATA_1	0x48
#define VA_INTR_R_DATA_2	0x50
#define VA_INTR_R_DATA_3	0x58
#define VA_INTR_R_DATA_4	0x60
#define VA_INTR_R_DATA_5	0x68
#define VA_INTR_R_DATA_6	0x80
#define VA_INTR_R_DATA_7	0x88
#endif

/* Shifts in the Interrupt Vector Dispatch virtual address. */
#define INTR_VEC_DISPATCH_MID_SHIFT	14

/* Bits in the Interrupt Dispatch Status register. */
#define INTR_DISPATCH_STATUS_NACK	0x2
#define INTR_DISPATCH_STATUS_BUSY	0x1

#define TT_INTERRUPT_VECTOR_TRAP		0x60

#define INTERRUPT_VECTOR_TRAP_HANDLER_SIZE	TRAP_TABLE_ENTRY_SIZE

#endif

/** @}
 */
