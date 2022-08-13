/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#include <typedefs.h>

// NOTE: This struct must match assembly code in src/context.S

/*
 * Thread context containing registers that must be preserved across
 * function calls.
 */
typedef struct context {
	uint32_t cpu_mode;
	uintptr_t sp;
	uintptr_t pc;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	/* r11 */
	uint32_t fp;
	ipl_t ipl;
} context_t;

#endif
