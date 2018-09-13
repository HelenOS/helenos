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

#define ISTATE_OFFSET_A0         0x00
#define ISTATE_OFFSET_A1         0x04
#define ISTATE_OFFSET_A2         0x08
#define ISTATE_OFFSET_A3         0x0c
#define ISTATE_OFFSET_T0         0x10
#define ISTATE_OFFSET_T1         0x14
#define ISTATE_OFFSET_V0         0x18
#define ISTATE_OFFSET_V1         0x1c
#define ISTATE_OFFSET_AT         0x20
#define ISTATE_OFFSET_T2         0x24
#define ISTATE_OFFSET_T3         0x28
#define ISTATE_OFFSET_T4         0x2c
#define ISTATE_OFFSET_T5         0x30
#define ISTATE_OFFSET_T6         0x34
#define ISTATE_OFFSET_T7         0x38
#define ISTATE_OFFSET_S0         0x3c
#define ISTATE_OFFSET_S1         0x40
#define ISTATE_OFFSET_S2         0x44
#define ISTATE_OFFSET_S3         0x48
#define ISTATE_OFFSET_S4         0x4c
#define ISTATE_OFFSET_S5         0x50
#define ISTATE_OFFSET_S6         0x54
#define ISTATE_OFFSET_S7         0x58
#define ISTATE_OFFSET_T8         0x5c
#define ISTATE_OFFSET_T9         0x60
#define ISTATE_OFFSET_KT0        0x64
#define ISTATE_OFFSET_KT1        0x68
#define ISTATE_OFFSET_GP         0x6c
#define ISTATE_OFFSET_SP         0x70
#define ISTATE_OFFSET_S8         0x74
#define ISTATE_OFFSET_RA         0x78
#define ISTATE_OFFSET_LO         0x7c
#define ISTATE_OFFSET_HI         0x80
#define ISTATE_OFFSET_STATUS     0x84
#define ISTATE_OFFSET_EPC        0x88
#define ISTATE_OFFSET_ALIGNMENT  0x8c
#define ISTATE_SIZE              0x90

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef KERNEL
#include <typedefs.h>
#else
#include <stddef.h>
#endif

typedef struct istate {
	/*
	 * The first seven registers are arranged so that the istate structure
	 * can be used both for exception handlers and for the syscall handler.
	 */

	uint32_t a0;  // arg1
	uint32_t a1;  // arg2
	uint32_t a2;  // arg3
	uint32_t a3;  // arg4
	uint32_t t0;  // arg5
	uint32_t t1;  // arg6
	uint32_t v0;  // arg7
	uint32_t v1;
	uint32_t at;
	uint32_t t2;
	uint32_t t3;
	uint32_t t4;
	uint32_t t5;
	uint32_t t6;
	uint32_t t7;
	uint32_t s0;
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
	uint32_t s4;
	uint32_t s5;
	uint32_t s6;
	uint32_t s7;
	uint32_t t8;
	uint32_t t9;
	uint32_t kt0;
	/* We use it as thread-local pointer */
	uint32_t kt1;
	uint32_t gp;
	uint32_t sp;
	uint32_t s8;
	uint32_t ra;
	uint32_t lo;
	uint32_t hi;
	/* cp0_status */
	uint32_t status;
	/* cp0_epc */
	uint32_t epc;
	/* to make sizeof(istate_t) a multiple of 8 */
	uint32_t alignment;
} istate_t;

#endif
#endif
