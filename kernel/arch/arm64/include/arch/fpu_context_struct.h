/*
 * SPDX-FileCopyrightText: 2018 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_FPU_CONTEXT_STRUCT_H_
#define KERN_ARCH_FPU_CONTEXT_STRUCT_H_

#define FPU_CONTEXT_OFFSET_VREGS  0x000
#define FPU_CONTEXT_OFFSET_FPCR   0x200
#define FPU_CONTEXT_OFFSET_FPSR   0x204
#define FPU_CONTEXT_SIZE          0x210

#ifndef __ASSEMBLER__

#include <typedefs.h>
#include <_bits/int128_t.h>

/** ARM64 FPU context. */
typedef struct fpu_context {
	uint128_t vregs[32];
	uint32_t fpcr;
	uint32_t fpsr;
} fpu_context_t;

#endif
#endif
