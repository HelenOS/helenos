/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x08
#define CONTEXT_OFFSET_GP   0x10
#define CONTEXT_OFFSET_TP   0x18
#define CONTEXT_OFFSET_S0   0x20
#define CONTEXT_OFFSET_S1   0x28
#define CONTEXT_OFFSET_S2   0x30
#define CONTEXT_OFFSET_S3   0x38
#define CONTEXT_OFFSET_S4   0x40
#define CONTEXT_OFFSET_S5   0x48
#define CONTEXT_OFFSET_S6   0x50
#define CONTEXT_OFFSET_S7   0x58
#define CONTEXT_OFFSET_S8   0x60
#define CONTEXT_OFFSET_S9   0x68
#define CONTEXT_OFFSET_S10  0x70
#define CONTEXT_OFFSET_S11  0x78
#define CONTEXT_OFFSET_IPL  0x80
#define CONTEXT_SIZE        0x88

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct context {
	uint64_t sp;
	uint64_t pc;
	uint64_t gp;
	uint64_t tp;
	uint64_t s0;
	uint64_t s1;
	uint64_t s2;
	uint64_t s3;
	uint64_t s4;
	uint64_t s5;
	uint64_t s6;
	uint64_t s7;
	uint64_t s8;
	uint64_t s9;
	uint64_t s10;
	uint64_t s11;
	ipl_t ipl;
} context_t;

#endif
#endif
