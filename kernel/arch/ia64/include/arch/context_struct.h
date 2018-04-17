/* Copyright (c) 2014 Jakub Jermar
 * All rights preserved.
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

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_AR_PFS          0x00
#define CONTEXT_OFFSET_AR_UNAT_CALLER  0x08
#define CONTEXT_OFFSET_AR_UNAT_CALLEE  0x10
#define CONTEXT_OFFSET_AR_RSC          0x18
#define CONTEXT_OFFSET_BSP             0x20
#define CONTEXT_OFFSET_AR_RNAT         0x28
#define CONTEXT_OFFSET_AR_LC           0x30
#define CONTEXT_OFFSET_R1              0x38
#define CONTEXT_OFFSET_R4              0x40
#define CONTEXT_OFFSET_R5              0x48
#define CONTEXT_OFFSET_R6              0x50
#define CONTEXT_OFFSET_R7              0x58
#define CONTEXT_OFFSET_SP              0x60
#define CONTEXT_OFFSET_R13             0x68
#define CONTEXT_OFFSET_PC              0x70
#define CONTEXT_OFFSET_B1              0x78
#define CONTEXT_OFFSET_B2              0x80
#define CONTEXT_OFFSET_B3              0x88
#define CONTEXT_OFFSET_B4              0x90
#define CONTEXT_OFFSET_B5              0x98
#define CONTEXT_OFFSET_PR              0xa0
#define CONTEXT_OFFSET_F2              0xb0
#define CONTEXT_OFFSET_F3              0xc0
#define CONTEXT_OFFSET_F4              0xd0
#define CONTEXT_OFFSET_F5              0xe0
#define CONTEXT_OFFSET_F16             0xf0
#define CONTEXT_OFFSET_F17             0x100
#define CONTEXT_OFFSET_F18             0x110
#define CONTEXT_OFFSET_F19             0x120
#define CONTEXT_OFFSET_F20             0x130
#define CONTEXT_OFFSET_F21             0x140
#define CONTEXT_OFFSET_F22             0x150
#define CONTEXT_OFFSET_F23             0x160
#define CONTEXT_OFFSET_F24             0x170
#define CONTEXT_OFFSET_F25             0x180
#define CONTEXT_OFFSET_F26             0x190
#define CONTEXT_OFFSET_F27             0x1a0
#define CONTEXT_OFFSET_F28             0x1b0
#define CONTEXT_OFFSET_F29             0x1c0
#define CONTEXT_OFFSET_F30             0x1d0
#define CONTEXT_OFFSET_F31             0x1e0
#define CONTEXT_OFFSET_IPL             0x1f0
#define CONTEXT_SIZE                   0x200

#ifndef __ASSEMBLER__

#include <typedefs.h>

// Only save registers that must be preserved across function calls.
typedef struct context {
	// Application registers.
	uint64_t ar_pfs;
	uint64_t ar_unat_caller;
	uint64_t ar_unat_callee;
	uint64_t ar_rsc;
	// ar_bsp
	uint64_t bsp;
	uint64_t ar_rnat;
	uint64_t ar_lc;

	// General registers.
	uint64_t r1;
	uint64_t r4;
	uint64_t r5;
	uint64_t r6;
	uint64_t r7;
	// r12
	uint64_t sp;
	uint64_t r13;

	// Branch registers.
	// b0
	uint64_t pc;
	uint64_t b1;
	uint64_t b2;
	uint64_t b3;
	uint64_t b4;
	uint64_t b5;

	// Predicate registers.
	uint64_t pr;
	uint128_t f2;
	uint128_t f3;
	uint128_t f4;
	uint128_t f5;
	uint128_t f16;
	uint128_t f17;
	uint128_t f18;
	uint128_t f19;
	uint128_t f20;
	uint128_t f21;
	uint128_t f22;
	uint128_t f23;
	uint128_t f24;
	uint128_t f25;
	uint128_t f26;
	uint128_t f27;
	uint128_t f28;
	uint128_t f29;
	uint128_t f30;
	uint128_t f31;

	ipl_t ipl;
} context_t;

#endif /* __ASSEMBLER__ */
#endif

