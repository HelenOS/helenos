/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_ARCH_FIBRIL_CONTEXT_H_
#define _LIBC_ARCH_FIBRIL_CONTEXT_H_

#define __CONTEXT_OFFSET_SP   0x00
#define __CONTEXT_OFFSET_PC   0x08
#define __CONTEXT_OFFSET_RBX  0x10
#define __CONTEXT_OFFSET_RBP  0x18
#define __CONTEXT_OFFSET_R12  0x20
#define __CONTEXT_OFFSET_R13  0x28
#define __CONTEXT_OFFSET_R14  0x30
#define __CONTEXT_OFFSET_R15  0x38
#define __CONTEXT_OFFSET_TLS  0x40
#define __CONTEXT_SIZE        0x48

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct __context {
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
	uint64_t tls;
} __context_t;

#endif
#endif
