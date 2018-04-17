/* Copyright (c) 2016 Martin Decky
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

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x08
#define CONTEXT_OFFSET_ZERO 0x10
#define CONTEXT_OFFSET_RA   0x18
#define CONTEXT_OFFSET_X3   0x20
#define CONTEXT_OFFSET_X4   0x28
#define CONTEXT_OFFSET_X5   0x30
#define CONTEXT_OFFSET_X6   0x38
#define CONTEXT_OFFSET_X7   0x40
#define CONTEXT_OFFSET_X8   0x48
#define CONTEXT_OFFSET_X9   0x50
#define CONTEXT_OFFSET_X10  0x58
#define CONTEXT_OFFSET_X11  0x60
#define CONTEXT_OFFSET_X12  0x68
#define CONTEXT_OFFSET_X13  0x70
#define CONTEXT_OFFSET_X14  0x78
#define CONTEXT_OFFSET_X15  0x80
#define CONTEXT_OFFSET_X16  0x88
#define CONTEXT_OFFSET_X17  0x90
#define CONTEXT_OFFSET_X18  0x98
#define CONTEXT_OFFSET_X19  0xa0
#define CONTEXT_OFFSET_X20  0xa8
#define CONTEXT_OFFSET_X21  0xb0
#define CONTEXT_OFFSET_X22  0xb8
#define CONTEXT_OFFSET_X23  0xc0
#define CONTEXT_OFFSET_X24  0xc8
#define CONTEXT_OFFSET_X25  0xd0
#define CONTEXT_OFFSET_X26  0xd8
#define CONTEXT_OFFSET_X27  0xe0
#define CONTEXT_OFFSET_X28  0xe8
#define CONTEXT_OFFSET_X29  0xf0
#define CONTEXT_OFFSET_X30  0xf8
#define CONTEXT_OFFSET_X31  0x100
#define CONTEXT_SIZE        0x108

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

/*
 * There is a room for optimization (we can store just those
 * registers that must be preserved during ABI function call).
 */

typedef struct context {
	uint64_t sp;
	uint64_t pc;
	uint64_t zero;
	uint64_t ra;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;
	uint64_t x30;
	uint64_t x31;
} context_t;

#endif
#endif

