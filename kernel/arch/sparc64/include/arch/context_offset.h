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

#ifndef KERN_sparc64_CONTEXT_OFFSET_H_
#define KERN_sparc64_CONTEXT_OFFSET_H_

#define OFFSET_SP       0x0
#define OFFSET_PC       0x8
#define OFFSET_I0       0x10
#define OFFSET_I1       0x18
#define OFFSET_I2       0x20
#define OFFSET_I3       0x28
#define OFFSET_I4       0x30
#define OFFSET_I5       0x38
#define OFFSET_FP       0x40
#define OFFSET_I7       0x48
#define OFFSET_L0       0x50
#define OFFSET_L1       0x58
#define OFFSET_L2       0x60
#define OFFSET_L3       0x68
#define OFFSET_L4       0x70
#define OFFSET_L5       0x78
#define OFFSET_L6       0x80
#define OFFSET_L7       0x88

#ifndef KERNEL		
# define OFFSET_TP      0x90
#endif

#ifdef __ASM__ 

.macro CONTEXT_SAVE_ARCH_CORE ctx:req
	stx %sp, [\ctx + OFFSET_SP]
	stx %o7, [\ctx + OFFSET_PC]
	stx %i0, [\ctx + OFFSET_I0]
	stx %i1, [\ctx + OFFSET_I1]
	stx %i2, [\ctx + OFFSET_I2]
	stx %i3, [\ctx + OFFSET_I3]
	stx %i4, [\ctx + OFFSET_I4]
	stx %i5, [\ctx + OFFSET_I5]
	stx %fp, [\ctx + OFFSET_FP]
	stx %i7, [\ctx + OFFSET_I7]
	stx %l0, [\ctx + OFFSET_L0]
	stx %l1, [\ctx + OFFSET_L1]
	stx %l2, [\ctx + OFFSET_L2]
	stx %l3, [\ctx + OFFSET_L3]
	stx %l4, [\ctx + OFFSET_L4]
	stx %l5, [\ctx + OFFSET_L5]
	stx %l6, [\ctx + OFFSET_L6]
	stx %l7, [\ctx + OFFSET_L7]
#ifndef KERNEL		
	stx %g7, [\ctx + OFFSET_TP]
#endif
.endm

.macro CONTEXT_RESTORE_ARCH_CORE ctx:req
	ldx [\ctx + OFFSET_SP], %sp
	ldx [\ctx + OFFSET_PC], %o7
	ldx [\ctx + OFFSET_I0], %i0
	ldx [\ctx + OFFSET_I1], %i1
	ldx [\ctx + OFFSET_I2], %i2
	ldx [\ctx + OFFSET_I3], %i3
	ldx [\ctx + OFFSET_I4], %i4
	ldx [\ctx + OFFSET_I5], %i5
	ldx [\ctx + OFFSET_FP], %fp
	ldx [\ctx + OFFSET_I7], %i7
	ldx [\ctx + OFFSET_L0], %l0
	ldx [\ctx + OFFSET_L1], %l1
	ldx [\ctx + OFFSET_L2], %l2
	ldx [\ctx + OFFSET_L3], %l3
	ldx [\ctx + OFFSET_L4], %l4
	ldx [\ctx + OFFSET_L5], %l5
	ldx [\ctx + OFFSET_L6], %l6
	ldx [\ctx + OFFSET_L7], %l7
#ifndef KERNEL		
	ldx [\ctx + OFFSET_TP], %g7
#endif
.endm

#endif /* __ASM__ */ 

#endif
