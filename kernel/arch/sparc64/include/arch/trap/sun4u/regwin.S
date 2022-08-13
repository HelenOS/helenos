/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_interrupt
 * @{
 */

/*
 * Macro used to spill userspace window to userspace window buffer.
 * It can be either triggered from preemptible_handler doing SAVE
 * at (TL=1) or from normal kernel code doing SAVE when OTHERWIN>0
 * at (TL=0).
 */
.macro SPILL_TO_USPACE_WINDOW_BUFFER
	stx %l0, [%g7 + L0_OFFSET]
	stx %l1, [%g7 + L1_OFFSET]
	stx %l2, [%g7 + L2_OFFSET]
	stx %l3, [%g7 + L3_OFFSET]
	stx %l4, [%g7 + L4_OFFSET]
	stx %l5, [%g7 + L5_OFFSET]
	stx %l6, [%g7 + L6_OFFSET]
	stx %l7, [%g7 + L7_OFFSET]
	stx %i0, [%g7 + I0_OFFSET]
	stx %i1, [%g7 + I1_OFFSET]
	stx %i2, [%g7 + I2_OFFSET]
	stx %i3, [%g7 + I3_OFFSET]
	stx %i4, [%g7 + I4_OFFSET]
	stx %i5, [%g7 + I5_OFFSET]
	stx %i6, [%g7 + I6_OFFSET]
	stx %i7, [%g7 + I7_OFFSET]
	add %g7, STACK_WINDOW_SAVE_AREA_SIZE, %g7
	saved
	retry
.endm

/** @}
 */
