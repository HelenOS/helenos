/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_FPU_CONTEXT_STRUCT_H_
#define KERN_ARCH_FPU_CONTEXT_STRUCT_H_

#define FPU_CONTEXT_OFFSET_DREGS  0
#define FPU_CONTEXT_OFFSET_CREGS  128
#define FPU_CONTEXT_SIZE          256

#define FPU_CONTEXT_SIZE_DREGS 128
#define FPU_CONTEXT_SIZE_CREGS 128

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct fpu_context {
	sysarg_t dregs[32];
	sysarg_t cregs[32];
} fpu_context_t;

#endif
#endif
