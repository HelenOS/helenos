/*
 * Copyright (c) 2005 Martin Decky
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

#ifndef KERN_ppc32_CONTEXT_OFFSET_H_
#define KERN_ppc32_CONTEXT_OFFSET_H_

#define OFFSET_SP    0x0
#define OFFSET_PC    0x4
#define OFFSET_R2    0x8
#define OFFSET_R13   0xc
#define OFFSET_R14   0x10
#define OFFSET_R15   0x14
#define OFFSET_R16   0x18
#define OFFSET_R17   0x1c
#define OFFSET_R18   0x20
#define OFFSET_R19   0x24
#define OFFSET_R20   0x28
#define OFFSET_R21   0x2c
#define OFFSET_R22   0x30
#define OFFSET_R23   0x34
#define OFFSET_R24   0x38
#define OFFSET_R25   0x3c
#define OFFSET_R26   0x40
#define OFFSET_R27   0x44
#define OFFSET_R28   0x48
#define OFFSET_R29   0x4c
#define OFFSET_R30   0x50
#define OFFSET_R31   0x54
#define OFFSET_CR    0x58

#define OFFSET_FR14  0x0
#define OFFSET_FR15  0x8
#define OFFSET_FR16  0x10
#define OFFSET_FR17  0x18
#define OFFSET_FR18  0x20
#define OFFSET_FR19  0x28
#define OFFSET_FR20  0x30
#define OFFSET_FR21  0x38
#define OFFSET_FR22  0x40
#define OFFSET_FR23  0x48
#define OFFSET_FR24  0x50
#define OFFSET_FR25  0x58
#define OFFSET_FR26  0x60
#define OFFSET_FR27  0x68
#define OFFSET_FR28  0x70
#define OFFSET_FR29  0x78
#define OFFSET_FR30  0x80
#define OFFSET_FR31  0x88
#define OFFSET_FPSCR 0x90

#ifdef __ASM__

#ifdef KERNEL

#include <arch/asm/regname.h>

#else /* KERNEL */

#include <libarch/regname.h>

#endif /* KERNEL */

/* ctx: address of the structure with saved context */
.macro CONTEXT_SAVE_ARCH_CORE ctx:req
	stw sp, OFFSET_SP(\ctx)
	stw r2, OFFSET_R2(\ctx)
	stw r13, OFFSET_R13(\ctx)
	stw r14, OFFSET_R14(\ctx)
	stw r15, OFFSET_R15(\ctx)
	stw r16, OFFSET_R16(\ctx)
	stw r17, OFFSET_R17(\ctx)
	stw r18, OFFSET_R18(\ctx)
	stw r19, OFFSET_R19(\ctx)
	stw r20, OFFSET_R20(\ctx)
	stw r21, OFFSET_R21(\ctx)
	stw r22, OFFSET_R22(\ctx)
	stw r23, OFFSET_R23(\ctx)
	stw r24, OFFSET_R24(\ctx)
	stw r25, OFFSET_R25(\ctx)
	stw r26, OFFSET_R26(\ctx)
	stw r27, OFFSET_R27(\ctx)
	stw r28, OFFSET_R28(\ctx)
	stw r29, OFFSET_R29(\ctx)
	stw r30, OFFSET_R30(\ctx)
	stw r31, OFFSET_R31(\ctx)
.endm

/* ctx: address of the structure with saved context */
.macro CONTEXT_RESTORE_ARCH_CORE ctx:req
	lwz sp, OFFSET_SP(\ctx)
	lwz r2, OFFSET_R2(\ctx)
	lwz r13, OFFSET_R13(\ctx)
	lwz r14, OFFSET_R14(\ctx)
	lwz r15, OFFSET_R15(\ctx)
	lwz r16, OFFSET_R16(\ctx)
	lwz r17, OFFSET_R17(\ctx)
	lwz r18, OFFSET_R18(\ctx)
	lwz r19, OFFSET_R19(\ctx)
	lwz r20, OFFSET_R20(\ctx)
	lwz r21, OFFSET_R21(\ctx)
	lwz r22, OFFSET_R22(\ctx)
	lwz r23, OFFSET_R23(\ctx)
	lwz r24, OFFSET_R24(\ctx)
	lwz r25, OFFSET_R25(\ctx)
	lwz r26, OFFSET_R26(\ctx)
	lwz r27, OFFSET_R27(\ctx)
	lwz r28, OFFSET_R28(\ctx)
	lwz r29, OFFSET_R29(\ctx)
	lwz r30, OFFSET_R30(\ctx)
	lwz r31, OFFSET_R31(\ctx)
.endm

#endif /* __ASM__ */

#endif
