/* Copyright (c) 2014 Jakub Jermar
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

#ifndef LIBC_ARCH_FIBRIL_CONTEXT_H_
#define LIBC_ARCH_FIBRIL_CONTEXT_H_

#define CONTEXT_OFFSET_SP  0x00
#define CONTEXT_OFFSET_PC  0x08
#define CONTEXT_OFFSET_I0  0x10
#define CONTEXT_OFFSET_I1  0x18
#define CONTEXT_OFFSET_I2  0x20
#define CONTEXT_OFFSET_I3  0x28
#define CONTEXT_OFFSET_I4  0x30
#define CONTEXT_OFFSET_I5  0x38
#define CONTEXT_OFFSET_FP  0x40
#define CONTEXT_OFFSET_I7  0x48
#define CONTEXT_OFFSET_L0  0x50
#define CONTEXT_OFFSET_L1  0x58
#define CONTEXT_OFFSET_L2  0x60
#define CONTEXT_OFFSET_L3  0x68
#define CONTEXT_OFFSET_L4  0x70
#define CONTEXT_OFFSET_L5  0x78
#define CONTEXT_OFFSET_L6  0x80
#define CONTEXT_OFFSET_L7  0x88
#define CONTEXT_OFFSET_TP  0x90
#define CONTEXT_SIZE       0x98

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

typedef struct context {
	uintptr_t sp;  // %o6
	uintptr_t pc;  // %o7
	uint64_t i0;
	uint64_t i1;
	uint64_t i2;
	uint64_t i3;
	uint64_t i4;
	uint64_t i5;
	uintptr_t fp;  // %i6
	uintptr_t i7;
	uint64_t l0;
	uint64_t l1;
	uint64_t l2;
	uint64_t l3;
	uint64_t l4;
	uint64_t l5;
	uint64_t l6;
	uint64_t l7;
	uint64_t tp;  // %g7
} context_t;

#endif
#endif

