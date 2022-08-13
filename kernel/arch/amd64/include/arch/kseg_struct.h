/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_KSEG_STRUCT_H_
#define KERN_ARCH_KSEG_STRUCT_H_

#define KSEG_OFFSET_USTACK_RSP  0x00
#define KSEG_OFFSET_KSTACK_RSP  0x08
#define KSEG_OFFSET_FSBASE      0x10
#define KSEG_SIZE               0x18

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct kseg {
	uint64_t ustack_rsp;
	uint64_t kstack_rsp;
	uint64_t fsbase;
} kseg_t;

#endif
#endif
