/*
 * SPDX-FileCopyrightText: 2008 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/mm/mmu.h>
#include <arch/regdef.h>
#include <arch/stack.h>

.text

/** Switch to userspace.
 *
 * %o0	Userspace entry address.
 * %o1	Userspace stack pointer address.
 * %o2  Userspace address of uarg structure.
 */
FUNCTION_BEGIN(switch_to_userspace)
	save %o1, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	wrpr %g0, 0, %cleanwin		! avoid information leak

	mov %i2, %o0			! uarg
	xor %o1, %o1, %o1		! %o1 is defined to hold pcb_ptr
					! set it to 0
	clr %i2
	clr %i3
	clr %i4
	clr %i5
	clr %i6
	wrpr %g0, 1, %tl		! enforce mapping via nucleus

	rdpr %cwp, %g1
	wrpr %g1, TSTATE_IE_BIT, %tstate
	wrpr %i0, 0, %tnpc

	/*
	 * Set primary context according to secondary context.
	 * Secondary context has been already installed by
	 * higher-level functions.
	 */
	wr %g0, ASI_SECONDARY_CONTEXT_REG, %asi
	ldxa [VA_SECONDARY_CONTEXT_REG] %asi, %g1
	wr %g0, ASI_PRIMARY_CONTEXT_REG, %asi
	stxa %g1, [VA_PRIMARY_CONTEXT_REG] %asi
	flush %i7

	/*
	 * Spills and fills will be handled by the userspace handlers.
	 */
	wrpr %g0, WSTATE_OTHER(0) | WSTATE_NORMAL(1), %wstate
	done				! jump to userspace
FUNCTION_END(switch_to_userspace)
