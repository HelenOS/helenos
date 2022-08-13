/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP  0x00
#define CONTEXT_OFFSET_PC  0x04
#define CONTEXT_OFFSET_S0  0x08
#define CONTEXT_OFFSET_S1  0x0c
#define CONTEXT_OFFSET_S2  0x10
#define CONTEXT_OFFSET_S3  0x14
#define CONTEXT_OFFSET_S4  0x18
#define CONTEXT_OFFSET_S5  0x1c
#define CONTEXT_OFFSET_S6  0x20
#define CONTEXT_OFFSET_S7  0x24
#define CONTEXT_OFFSET_S8  0x28
#define CONTEXT_OFFSET_GP  0x2c
#define CONTEXT_OFFSET_TP  0x30
#define CONTEXT_OFFSET_IPL 0x34
#define CONTEXT_SIZE       0x38

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct context {
	/* Only save registers that must be preserved across function calls. */
	uint32_t sp;
	uint32_t pc;
	uint32_t s0;
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
	uint32_t s4;
	uint32_t s5;
	uint32_t s6;
	uint32_t s7;
	uint32_t s8;
	uint32_t gp;
	/* We use the K1 register for userspace thread pointer. */
	uint32_t tp;
	ipl_t ipl;
} context_t;

#endif
#endif
