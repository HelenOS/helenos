/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_TRAP_TABLE_H_
#define KERN_sparc64_TRAP_TABLE_H_

#include <arch/stack.h>
#include <arch/istate_struct.h>

#define TRAP_TABLE_ENTRY_COUNT	1024
#define TRAP_TABLE_ENTRY_SIZE	32
#define TRAP_TABLE_SIZE		(TRAP_TABLE_ENTRY_COUNT * TRAP_TABLE_ENTRY_SIZE)

#ifdef __ASSEMBLER__
#include <arch/trap/trap_table.S>
#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
