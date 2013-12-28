/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2013 Jakub Klama
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

#ifndef KERN_sparc32_CONTEXT_OFFSET_H_
#define KERN_sparc32_CONTEXT_OFFSET_H_

#define OFFSET_SP  0
#define OFFSET_PC  4
#define OFFSET_I0  8
#define OFFSET_I1  12
#define OFFSET_I2  16
#define OFFSET_I3  20
#define OFFSET_I4  24
#define OFFSET_I5  28
#define OFFSET_FP  32
#define OFFSET_I7  36
#define OFFSET_L0  40
#define OFFSET_L1  44
#define OFFSET_L2  48
#define OFFSET_L3  52
#define OFFSET_L4  56
#define OFFSET_L5  60
#define OFFSET_L6  64
#define OFFSET_L7  68

#ifndef KERNEL

#define OFFSET_TP  72

#endif

#ifdef __ASM__

.macro CONTEXT_SAVE_ARCH_CORE ctx:req
	st %sp, [\ctx + OFFSET_SP]
	st %o7, [\ctx + OFFSET_PC]
	st %i0, [\ctx + OFFSET_I0]
	st %i1, [\ctx + OFFSET_I1]
	st %i2, [\ctx + OFFSET_I2]
	st %i3, [\ctx + OFFSET_I3]
	st %i4, [\ctx + OFFSET_I4]
	st %i5, [\ctx + OFFSET_I5]
	st %fp, [\ctx + OFFSET_FP]
	st %i7, [\ctx + OFFSET_I7]
	st %l0, [\ctx + OFFSET_L0]
	st %l1, [\ctx + OFFSET_L1]
	st %l2, [\ctx + OFFSET_L2]
	st %l3, [\ctx + OFFSET_L3]
	st %l4, [\ctx + OFFSET_L4]
	st %l5, [\ctx + OFFSET_L5]
	st %l6, [\ctx + OFFSET_L6]
	st %l7, [\ctx + OFFSET_L7]
#ifndef KERNEL
	st %g7, [\ctx + OFFSET_TP]
#endif
.endm

.macro CONTEXT_RESTORE_ARCH_CORE ctx:req
	ld [\ctx + OFFSET_SP], %sp
	ld [\ctx + OFFSET_PC], %o7
	ld [\ctx + OFFSET_I0], %i0
	ld [\ctx + OFFSET_I1], %i1
	ld [\ctx + OFFSET_I2], %i2
	ld [\ctx + OFFSET_I3], %i3
	ld [\ctx + OFFSET_I4], %i4
	ld [\ctx + OFFSET_I5], %i5
	ld [\ctx + OFFSET_FP], %fp
	ld [\ctx + OFFSET_I7], %i7
	ld [\ctx + OFFSET_L0], %l0
	ld [\ctx + OFFSET_L1], %l1
	ld [\ctx + OFFSET_L2], %l2
	ld [\ctx + OFFSET_L3], %l3
	ld [\ctx + OFFSET_L4], %l4
	ld [\ctx + OFFSET_L5], %l5
	ld [\ctx + OFFSET_L6], %l6
	ld [\ctx + OFFSET_L7], %l7
#ifndef KERNEL
	ld [\ctx + OFFSET_TP], %g7
#endif
.endm

#endif /* __ASM__ */

#endif
