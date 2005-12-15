/*
 * Copyright (C) 2005 Jakub Jermar
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

/**
 * This file contains register window trap handlers.
 */

#ifndef __sparc64_REGWIN_H__
#define __sparc64_REGWIN_H__

#include <arch/stack.h>

#define TT_CLEAN_WINDOW			0x24
#define TT_SPILL_0_NORMAL		0x80
#define TT_FILL_0_NORMAL		0xc0

#define REGWIN_HANDLER_SIZE		128

#define CLEAN_WINDOW_HANDLER_SIZE	REGWIN_HANDLER_SIZE
#define SPILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE
#define FILL_HANDLER_SIZE		REGWIN_HANDLER_SIZE

/** Window Save Area offsets. */
#define L0_OFFSET	0
#define L1_OFFSET	8
#define L2_OFFSET	16
#define L3_OFFSET	24
#define L4_OFFSET	32
#define L5_OFFSET	40
#define L6_OFFSET	48
#define L7_OFFSET	56
#define I0_OFFSET	64
#define I1_OFFSET	72
#define I2_OFFSET	80
#define I3_OFFSET	88
#define I4_OFFSET	96
#define I5_OFFSET	104
#define I6_OFFSET	112
#define I7_OFFSET	120

.macro SPILL_NORMAL_HANDLER
	stx %l0, [%sp + STACK_BIAS + L0_OFFSET]	
	stx %l1, [%sp + STACK_BIAS + L1_OFFSET]
	stx %l2, [%sp + STACK_BIAS + L2_OFFSET]
	stx %l3, [%sp + STACK_BIAS + L3_OFFSET]
	stx %l4, [%sp + STACK_BIAS + L4_OFFSET]
	stx %l5, [%sp + STACK_BIAS + L5_OFFSET]
	stx %l6, [%sp + STACK_BIAS + L6_OFFSET]
	stx %l7, [%sp + STACK_BIAS + L7_OFFSET]
	stx %i0, [%sp + STACK_BIAS + I0_OFFSET]
	stx %i1, [%sp + STACK_BIAS + I1_OFFSET]
	stx %i2, [%sp + STACK_BIAS + I2_OFFSET]
	stx %i3, [%sp + STACK_BIAS + I3_OFFSET]
	stx %i4, [%sp + STACK_BIAS + I4_OFFSET]
	stx %i5, [%sp + STACK_BIAS + I5_OFFSET]
	stx %i6, [%sp + STACK_BIAS + I6_OFFSET]
	stx %i7, [%sp + STACK_BIAS + I7_OFFSET]
	saved
	retry
.endm

.macro FILL_NORMAL_HANDLER
	ldx [%sp + STACK_BIAS + L0_OFFSET], %l0
	ldx [%sp + STACK_BIAS + L1_OFFSET], %l1
	ldx [%sp + STACK_BIAS + L2_OFFSET], %l2
	ldx [%sp + STACK_BIAS + L3_OFFSET], %l3
	ldx [%sp + STACK_BIAS + L4_OFFSET], %l4
	ldx [%sp + STACK_BIAS + L5_OFFSET], %l5
	ldx [%sp + STACK_BIAS + L6_OFFSET], %l6
	ldx [%sp + STACK_BIAS + L7_OFFSET], %l7
	ldx [%sp + STACK_BIAS + I0_OFFSET], %i0
	ldx [%sp + STACK_BIAS + I1_OFFSET], %i1
	ldx [%sp + STACK_BIAS + I2_OFFSET], %i2
	ldx [%sp + STACK_BIAS + I3_OFFSET], %i3
	ldx [%sp + STACK_BIAS + I4_OFFSET], %i4
	ldx [%sp + STACK_BIAS + I5_OFFSET], %i5
	ldx [%sp + STACK_BIAS + I6_OFFSET], %i6
	ldx [%sp + STACK_BIAS + I7_OFFSET], %i7
	restored
	retry
.endm

.macro CLEAN_WINDOW_HANDLER
	rdpr %cleanwin, %l0
	add %l0, 1, %l0
	wrpr %l0, 0, %cleanwin
	mov %r0, %l0
	mov %r0, %l1
	mov %r0, %l2
	mov %r0, %l3
	mov %r0, %l4
	mov %r0, %l5
	mov %r0, %l6
	mov %r0, %l7
	mov %r0, %i0
	mov %r0, %i1
	mov %r0, %i2
	mov %r0, %i3
	mov %r0, %i4
	mov %r0, %i5
	retry
.endm

#endif
