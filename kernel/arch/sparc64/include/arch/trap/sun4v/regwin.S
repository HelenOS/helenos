/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */

/*
 * Saves the contents of the current window to the userspace window buffer.
 * Does not modify any register window registers, but updates pointer to the
 * top of the userspace window buffer.
 *
 * Parameters:
 * 	\tmpreg1	global register to be used for scratching purposes
 * 	\tmpreg2	global register to be used for scratching purposes
 */
.macro SAVE_TO_USPACE_WBUF tmpreg1, tmpreg2
	set SCRATCHPAD_WBUF, \tmpreg2
	ldxa [\tmpreg2] ASI_SCRATCHPAD, \tmpreg1
	stx %l0, [\tmpreg1 + L0_OFFSET]
	stx %l1, [\tmpreg1 + L1_OFFSET]
	stx %l2, [\tmpreg1 + L2_OFFSET]
	stx %l3, [\tmpreg1 + L3_OFFSET]
	stx %l4, [\tmpreg1 + L4_OFFSET]
	stx %l5, [\tmpreg1 + L5_OFFSET]
	stx %l6, [\tmpreg1 + L6_OFFSET]
	stx %l7, [\tmpreg1 + L7_OFFSET]
	stx %i0, [\tmpreg1 + I0_OFFSET]
	stx %i1, [\tmpreg1 + I1_OFFSET]
	stx %i2, [\tmpreg1 + I2_OFFSET]
	stx %i3, [\tmpreg1 + I3_OFFSET]
	stx %i4, [\tmpreg1 + I4_OFFSET]
	stx %i5, [\tmpreg1 + I5_OFFSET]
	stx %i6, [\tmpreg1 + I6_OFFSET]
	stx %i7, [\tmpreg1 + I7_OFFSET]
	add \tmpreg1, STACK_WINDOW_SAVE_AREA_SIZE, \tmpreg1
	stxa \tmpreg1, [\tmpreg2] ASI_SCRATCHPAD
.endm

/*
 * Macro used to spill userspace window to userspace window buffer.
 * It is triggered from normal kernel code doing SAVE when
 * OTHERWIN>0 at (TL=0).
 */
.macro SPILL_TO_USPACE_WINDOW_BUFFER
	SAVE_TO_USPACE_WBUF %g7, %g4
	saved
	retry
.endm

/** @}
 */
