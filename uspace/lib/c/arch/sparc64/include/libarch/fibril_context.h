/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_ARCH_FIBRIL_CONTEXT_H_
#define _LIBC_ARCH_FIBRIL_CONTEXT_H_

#define __CONTEXT_OFFSET_SP  0x00
#define __CONTEXT_OFFSET_PC  0x08
#define __CONTEXT_OFFSET_I0  0x10
#define __CONTEXT_OFFSET_I1  0x18
#define __CONTEXT_OFFSET_I2  0x20
#define __CONTEXT_OFFSET_I3  0x28
#define __CONTEXT_OFFSET_I4  0x30
#define __CONTEXT_OFFSET_I5  0x38
#define __CONTEXT_OFFSET_FP  0x40
#define __CONTEXT_OFFSET_I7  0x48
#define __CONTEXT_OFFSET_L0  0x50
#define __CONTEXT_OFFSET_L1  0x58
#define __CONTEXT_OFFSET_L2  0x60
#define __CONTEXT_OFFSET_L3  0x68
#define __CONTEXT_OFFSET_L4  0x70
#define __CONTEXT_OFFSET_L5  0x78
#define __CONTEXT_OFFSET_L6  0x80
#define __CONTEXT_OFFSET_L7  0x88
#define __CONTEXT_OFFSET_TP  0x90
#define __CONTEXT_SIZE       0x98

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

typedef struct __context {
	uintptr_t sp;  // %o6
	uintptr_t pc;  // %o7
	uint64_t i0;
	uint64_t i1;
	uint64_t i2;
	uint64_t i3;
	uint64_t i4;
	uint64_t i5;
	uintptr_t fp;  // %i6
	uintptr_t i7;
	uint64_t l0;
	uint64_t l1;
	uint64_t l2;
	uint64_t l3;
	uint64_t l4;
	uint64_t l5;
	uint64_t l6;
	uint64_t l7;
	uint64_t tp;  // %g7
} __context_t;

#endif
#endif
