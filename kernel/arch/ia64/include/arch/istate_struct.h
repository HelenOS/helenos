/*
 * Copyright (c) 2014 Jakub Jermar
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

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_F2                0x000
#define ISTATE_OFFSET_F3                0x010
#define ISTATE_OFFSET_F4                0x020
#define ISTATE_OFFSET_F5                0x030
#define ISTATE_OFFSET_F6                0x040
#define ISTATE_OFFSET_F7                0x050
#define ISTATE_OFFSET_F8                0x060
#define ISTATE_OFFSET_F9                0x070
#define ISTATE_OFFSET_F10               0x080
#define ISTATE_OFFSET_F11               0x090
#define ISTATE_OFFSET_F12               0x0a0
#define ISTATE_OFFSET_F13               0x0b0
#define ISTATE_OFFSET_F14               0x0c0
#define ISTATE_OFFSET_F15               0x0d0
#define ISTATE_OFFSET_F16               0x0e0
#define ISTATE_OFFSET_F17               0x0f0
#define ISTATE_OFFSET_F18               0x100
#define ISTATE_OFFSET_F19               0x110
#define ISTATE_OFFSET_F20               0x120
#define ISTATE_OFFSET_F21               0x130
#define ISTATE_OFFSET_F22               0x140
#define ISTATE_OFFSET_F23               0x150
#define ISTATE_OFFSET_F24               0x160
#define ISTATE_OFFSET_F25               0x170
#define ISTATE_OFFSET_F26               0x180
#define ISTATE_OFFSET_F27               0x190
#define ISTATE_OFFSET_F28               0x1a0
#define ISTATE_OFFSET_F29               0x1b0
#define ISTATE_OFFSET_F30               0x1c0
#define ISTATE_OFFSET_F31               0x1d0
#define ISTATE_OFFSET_AR_BSP            0x1e0
#define ISTATE_OFFSET_AR_BSPSTORE       0x1e8
#define ISTATE_OFFSET_AR_BSPSTORE_NEW   0x1f0
#define ISTATE_OFFSET_AR_RNAT           0x1f8
#define ISTATE_OFFSET_AR_IFS            0x200
#define ISTATE_OFFSET_AR_PFS            0x208
#define ISTATE_OFFSET_AR_RSC            0x210
#define ISTATE_OFFSET_CR_IFA            0x218
#define ISTATE_OFFSET_CR_ISR            0x220
#define ISTATE_OFFSET_CR_IIPA           0x228
#define ISTATE_OFFSET_CR_IPSR           0x230
#define ISTATE_OFFSET_CR_IIP            0x238
#define ISTATE_OFFSET_CR_IIM            0x240
#define ISTATE_OFFSET_PR                0x248
#define ISTATE_OFFSET_SP                0x250
#define ISTATE_OFFSET_IN0               0x258
#define ISTATE_OFFSET_IN1               0x260
#define ISTATE_OFFSET_IN2               0x268
#define ISTATE_OFFSET_IN3               0x270
#define ISTATE_OFFSET_IN4               0x278
#define ISTATE_OFFSET_IN5               0x280
#define ISTATE_OFFSET_IN6               0x288
#define ISTATE_SIZE                     0x290

#ifndef __ASSEMBLER__

#include <stdint.h>
#include <_bits/int128_t.h>

#ifdef KERNEL
#include <typedefs.h>
#include <arch/register.h>
#else
#include <stddef.h>
#include <libarch/register.h>
#endif

typedef struct istate {
	uint128_t f2;
	uint128_t f3;
	uint128_t f4;
	uint128_t f5;
	uint128_t f6;
	uint128_t f7;
	uint128_t f8;
	uint128_t f9;
	uint128_t f10;
	uint128_t f11;
	uint128_t f12;
	uint128_t f13;
	uint128_t f14;
	uint128_t f15;
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
	uintptr_t ar_bsp;
	uintptr_t ar_bspstore;
	uintptr_t ar_bspstore_new;
	uint64_t ar_rnat;
	uint64_t ar_ifs;
	uint64_t ar_pfs;
	uint64_t ar_rsc;
	uintptr_t cr_ifa;
	cr_isr_t cr_isr;
	uintptr_t cr_iipa;
	psr_t cr_ipsr;
	uintptr_t cr_iip;
	uint64_t cr_iim;
	uint64_t pr;
	uintptr_t sp;

	// The following variables are defined only for break_instruction
	// handler.
	uint64_t in0;
	uint64_t in1;
	uint64_t in2;
	uint64_t in3;
	uint64_t in4;
	uint64_t in5;
	uint64_t in6;
} istate_t;

#endif
#endif
