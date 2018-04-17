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

#ifndef KERN_ARCH_FPU_CONTEXT_STRUCT_H_
#define KERN_ARCH_FPU_CONTEXT_STRUCT_H_

#define FPU_CONTEXT_OFFSET_FR0    0x00
#define FPU_CONTEXT_OFFSET_FR1    0x08
#define FPU_CONTEXT_OFFSET_FR2    0x10
#define FPU_CONTEXT_OFFSET_FR3    0x18
#define FPU_CONTEXT_OFFSET_FR4    0x20
#define FPU_CONTEXT_OFFSET_FR5    0x28
#define FPU_CONTEXT_OFFSET_FR6    0x30
#define FPU_CONTEXT_OFFSET_FR7    0x38
#define FPU_CONTEXT_OFFSET_FR8    0x40
#define FPU_CONTEXT_OFFSET_FR9    0x48
#define FPU_CONTEXT_OFFSET_FR10   0x50
#define FPU_CONTEXT_OFFSET_FR11   0x58
#define FPU_CONTEXT_OFFSET_FR12   0x60
#define FPU_CONTEXT_OFFSET_FR13   0x68
#define FPU_CONTEXT_OFFSET_FR14   0x70
#define FPU_CONTEXT_OFFSET_FR15   0x78
#define FPU_CONTEXT_OFFSET_FR16   0x80
#define FPU_CONTEXT_OFFSET_FR17   0x88
#define FPU_CONTEXT_OFFSET_FR18   0x90
#define FPU_CONTEXT_OFFSET_FR19   0x98
#define FPU_CONTEXT_OFFSET_FR20   0xa0
#define FPU_CONTEXT_OFFSET_FR21   0xa8
#define FPU_CONTEXT_OFFSET_FR22   0xb0
#define FPU_CONTEXT_OFFSET_FR23   0xb8
#define FPU_CONTEXT_OFFSET_FR24   0xc0
#define FPU_CONTEXT_OFFSET_FR25   0xc8
#define FPU_CONTEXT_OFFSET_FR26   0xd0
#define FPU_CONTEXT_OFFSET_FR27   0xd8
#define FPU_CONTEXT_OFFSET_FR28   0xe0
#define FPU_CONTEXT_OFFSET_FR29   0xe8
#define FPU_CONTEXT_OFFSET_FR30   0xf0
#define FPU_CONTEXT_OFFSET_FR31   0xf8
#define FPU_CONTEXT_OFFSET_FPSCR  0x100
#define FPU_CONTEXT_SIZE          0x108

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct fpu_context {
	uint64_t fr0;
	uint64_t fr1;
	uint64_t fr2;
	uint64_t fr3;
	uint64_t fr4;
	uint64_t fr5;
	uint64_t fr6;
	uint64_t fr7;
	uint64_t fr8;
	uint64_t fr9;
	uint64_t fr10;
	uint64_t fr11;
	uint64_t fr12;
	uint64_t fr13;
	uint64_t fr14;
	uint64_t fr15;
	uint64_t fr16;
	uint64_t fr17;
	uint64_t fr18;
	uint64_t fr19;
	uint64_t fr20;
	uint64_t fr21;
	uint64_t fr22;
	uint64_t fr23;
	uint64_t fr24;
	uint64_t fr25;
	uint64_t fr26;
	uint64_t fr27;
	uint64_t fr28;
	uint64_t fr29;
	uint64_t fr30;
	uint64_t fr31;
	uint64_t fpscr;
} fpu_context_t;

#endif
#endif

