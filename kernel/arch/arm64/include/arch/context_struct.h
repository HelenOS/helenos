/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x08
#define CONTEXT_OFFSET_X19  0x10
#define CONTEXT_OFFSET_X20  0x18
#define CONTEXT_OFFSET_X21  0x20
#define CONTEXT_OFFSET_X22  0x28
#define CONTEXT_OFFSET_X23  0x30
#define CONTEXT_OFFSET_X24  0x38
#define CONTEXT_OFFSET_X25  0x40
#define CONTEXT_OFFSET_X26  0x48
#define CONTEXT_OFFSET_X27  0x50
#define CONTEXT_OFFSET_X28  0x58
#define CONTEXT_OFFSET_X29  0x60
#define CONTEXT_OFFSET_IPL  0x68
#define CONTEXT_SIZE        0x70

#ifndef __ASSEMBLER__

#include <typedefs.h>

/*
 * Thread context containing registers that must be preserved across function
 * calls.
 */
typedef struct context {
	uint64_t sp;
	uint64_t pc;
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
	ipl_t ipl;
} context_t;

#endif
#endif
