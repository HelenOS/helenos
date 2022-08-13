/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x08
#define CONTEXT_OFFSET_RBX  0x10
#define CONTEXT_OFFSET_RBP  0x18
#define CONTEXT_OFFSET_R12  0x20
#define CONTEXT_OFFSET_R13  0x28
#define CONTEXT_OFFSET_R14  0x30
#define CONTEXT_OFFSET_R15  0x38
#define CONTEXT_OFFSET_TP   0x40
#define CONTEXT_OFFSET_IPL  0x48
#define CONTEXT_SIZE        0x50

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct context {
	/*
	 * We include only registers that must be preserved
	 * during function call.
	 */
	uint64_t sp;
	uint64_t pc;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t tp;
	ipl_t ipl;
} context_t;

#endif
#endif
