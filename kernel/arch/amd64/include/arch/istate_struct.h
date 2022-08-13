/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_RAX         0x00
#define ISTATE_OFFSET_RBX         0x08
#define ISTATE_OFFSET_RCX         0x10
#define ISTATE_OFFSET_RDX         0x18
#define ISTATE_OFFSET_RSI         0x20
#define ISTATE_OFFSET_RDI         0x28
#define ISTATE_OFFSET_RBP         0x30
#define ISTATE_OFFSET_R8          0x38
#define ISTATE_OFFSET_R9          0x40
#define ISTATE_OFFSET_R10         0x48
#define ISTATE_OFFSET_R11         0x50
#define ISTATE_OFFSET_R12         0x58
#define ISTATE_OFFSET_R13         0x60
#define ISTATE_OFFSET_R14         0x68
#define ISTATE_OFFSET_R15         0x70
#define ISTATE_OFFSET_ALIGNMENT   0x78
#define ISTATE_OFFSET_RBP_FRAME   0x80
#define ISTATE_OFFSET_RIP_FRAME   0x88
#define ISTATE_OFFSET_ERROR_WORD  0x90
#define ISTATE_OFFSET_RIP         0x98
#define ISTATE_OFFSET_CS          0xa0
#define ISTATE_OFFSET_RFLAGS      0xa8
#define ISTATE_OFFSET_RSP         0xb0
#define ISTATE_OFFSET_SS          0xb8
#define ISTATE_SIZE               0xc0

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct istate {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	/* align rbp_frame on multiple of 16 */
	uint64_t alignment;
	/* imitation of frame pointer linkage */
	uint64_t rbp_frame;
	/* imitation of frame address linkage */
	uint64_t rip_frame;
	/* real or fake error word */
	uint64_t error_word;
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	/* only if istate_t is from uspace */
	uint64_t rsp;
	/* only if istate_t is from uspace */
	uint64_t ss;
} istate_t;

#endif
#endif
