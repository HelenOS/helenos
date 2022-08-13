/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x04
#define CONTEXT_OFFSET_EBX  0x08
#define CONTEXT_OFFSET_ESI  0x0c
#define CONTEXT_OFFSET_EDI  0x10
#define CONTEXT_OFFSET_EBP  0x14
#define CONTEXT_OFFSET_TP   0x18
#define CONTEXT_OFFSET_IPL  0x1c
#define CONTEXT_SIZE        0x20

#ifndef __ASSEMBLER__

#include <typedefs.h>

/* Only save registers that must be preserved across function calls. */
typedef struct context {
	uint32_t sp;
	uint32_t pc;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t tp;
	ipl_t ipl;
} context_t;

#endif
#endif
