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

.macro SAVE_GLOBALS
	mov %g1, %l1
	mov %g2, %l2
	mov %g3, %l3
	mov %g4, %l4
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
.endm

.macro RESTORE_GLOBALS
	mov %l1, %g1
	mov %l2, %g2
	mov %l3, %g3
	mov %l4, %g4
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
.endm

.macro PREEMPTIBLE_HANDLER f
	sethi %hi(\f), %g1
	ba %xcc, preemptible_handler
	or %g1, %lo(\f), %g1
.endm

/** @}
 */
