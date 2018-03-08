/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup sparc64interrupt
 * @{
 */
#ifndef KERN_sparc64_sun4u_REGWIN_H_
#define KERN_sparc64_sun4u_REGWIN_H_

#ifdef __ASSEMBLER__

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

#endif

#endif

/** @}
 */
