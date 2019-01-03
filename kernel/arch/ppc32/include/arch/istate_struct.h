/*
 * Copyright (c) 2014 Jakub Jermar
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

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_SP_FRAME  0x00
#define ISTATE_OFFSET_LR_FRAME  0x04
#define ISTATE_OFFSET_R0        0x08
#define ISTATE_OFFSET_R2        0x0c
#define ISTATE_OFFSET_R3        0x10
#define ISTATE_OFFSET_R4        0x14
#define ISTATE_OFFSET_R5        0x18
#define ISTATE_OFFSET_R6        0x1c
#define ISTATE_OFFSET_R7        0x20
#define ISTATE_OFFSET_R8        0x24
#define ISTATE_OFFSET_R9        0x28
#define ISTATE_OFFSET_R10       0x2c
#define ISTATE_OFFSET_R11       0x30
#define ISTATE_OFFSET_R13       0x34
#define ISTATE_OFFSET_R14       0x38
#define ISTATE_OFFSET_R15       0x3c
#define ISTATE_OFFSET_R16       0x40
#define ISTATE_OFFSET_R17       0x44
#define ISTATE_OFFSET_R18       0x48
#define ISTATE_OFFSET_R19       0x4c
#define ISTATE_OFFSET_R20       0x50
#define ISTATE_OFFSET_R21       0x54
#define ISTATE_OFFSET_R22       0x58
#define ISTATE_OFFSET_R23       0x5c
#define ISTATE_OFFSET_R24       0x60
#define ISTATE_OFFSET_R25       0x64
#define ISTATE_OFFSET_R26       0x68
#define ISTATE_OFFSET_R27       0x6c
#define ISTATE_OFFSET_R28       0x70
#define ISTATE_OFFSET_R29       0x74
#define ISTATE_OFFSET_R30       0x78
#define ISTATE_OFFSET_R31       0x7c
#define ISTATE_OFFSET_CR        0x80
#define ISTATE_OFFSET_PC        0x84
#define ISTATE_OFFSET_SRR1      0x88
#define ISTATE_OFFSET_LR        0x8c
#define ISTATE_OFFSET_CTR       0x90
#define ISTATE_OFFSET_XER       0x94
#define ISTATE_OFFSET_DAR       0x98
#define ISTATE_OFFSET_R12       0x9c
#define ISTATE_OFFSET_SP        0xa0
#define ISTATE_SIZE             0xa4

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef KERNEL
#include <typedefs.h>
#else
#include <stddef.h>
#endif

typedef struct istate {
	/* imitation of frame pointer linkage */
	uint32_t sp_frame;
	/* imitation of return address linkage */
	uint32_t lr_frame;
	uint32_t r0;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t r27;
	uint32_t r28;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
	uint32_t cr;
	uint32_t pc;
	uint32_t srr1;
	uint32_t lr;
	uint32_t ctr;
	uint32_t xer;
	uint32_t dar;
	uint32_t r12;
	uint32_t sp;
} istate_t;

#endif
#endif
