/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_ZERO  0x00
#define ISTATE_OFFSET_RA    0x08
#define ISTATE_OFFSET_SP    0x10
#define ISTATE_OFFSET_X3    0x18
#define ISTATE_OFFSET_X4    0x20
#define ISTATE_OFFSET_X5    0x28
#define ISTATE_OFFSET_X6    0x30
#define ISTATE_OFFSET_X7    0x38
#define ISTATE_OFFSET_X8    0x40
#define ISTATE_OFFSET_X9    0x48
#define ISTATE_OFFSET_X10   0x50
#define ISTATE_OFFSET_X11   0x58
#define ISTATE_OFFSET_X12   0x60
#define ISTATE_OFFSET_X13   0x68
#define ISTATE_OFFSET_X14   0x70
#define ISTATE_OFFSET_X15   0x78
#define ISTATE_OFFSET_X16   0x80
#define ISTATE_OFFSET_X17   0x88
#define ISTATE_OFFSET_X18   0x90
#define ISTATE_OFFSET_X19   0x98
#define ISTATE_OFFSET_X20   0xa0
#define ISTATE_OFFSET_X21   0xa8
#define ISTATE_OFFSET_X22   0xb0
#define ISTATE_OFFSET_X23   0xb8
#define ISTATE_OFFSET_X24   0xc0
#define ISTATE_OFFSET_X25   0xc8
#define ISTATE_OFFSET_X26   0xd0
#define ISTATE_OFFSET_X27   0xd8
#define ISTATE_OFFSET_X28   0xe0
#define ISTATE_OFFSET_X29   0xe8
#define ISTATE_OFFSET_X30   0xf0
#define ISTATE_OFFSET_X31   0xf8
#define ISTATE_OFFSET_PC    0x100
#define ISTATE_SIZE         0x108

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef KERNEL
#include <typedefs.h>
#else
#include <stddef.h>
#endif

typedef struct istate {
	uint64_t zero;
	uint64_t ra;
	uint64_t sp;
	uint64_t x3;
	uint64_t x4;
	uint64_t x5;
	uint64_t x6;
	uint64_t x7;
	uint64_t x8;
	uint64_t x9;
	uint64_t x10;
	uint64_t x11;
	uint64_t x12;
	uint64_t x13;
	uint64_t x14;
	uint64_t x15;
	uint64_t x16;
	uint64_t x17;
	uint64_t x18;
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
	uint64_t x30;
	uint64_t x31;
	uint64_t pc;
} istate_t;

#endif
#endif
