/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x04
#define CONTEXT_OFFSET_R2   0x08
#define CONTEXT_OFFSET_R13  0x0c
#define CONTEXT_OFFSET_R14  0x10
#define CONTEXT_OFFSET_R15  0x14
#define CONTEXT_OFFSET_R16  0x18
#define CONTEXT_OFFSET_R17  0x1c
#define CONTEXT_OFFSET_R18  0x20
#define CONTEXT_OFFSET_R19  0x24
#define CONTEXT_OFFSET_R20  0x28
#define CONTEXT_OFFSET_R21  0x2c
#define CONTEXT_OFFSET_R22  0x30
#define CONTEXT_OFFSET_R23  0x34
#define CONTEXT_OFFSET_R24  0x38
#define CONTEXT_OFFSET_R25  0x3c
#define CONTEXT_OFFSET_R26  0x40
#define CONTEXT_OFFSET_R27  0x44
#define CONTEXT_OFFSET_R28  0x48
#define CONTEXT_OFFSET_R29  0x4c
#define CONTEXT_OFFSET_R30  0x50
#define CONTEXT_OFFSET_R31  0x54
#define CONTEXT_OFFSET_CR   0x58
#define CONTEXT_OFFSET_IPL  0x5c
#define CONTEXT_SIZE        0x60

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct context {
	uint32_t sp;
	uint32_t pc;
	uint32_t r2;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r24;
	uint32_t r25;
	uint32_t r26;
	uint32_t r27;
	uint32_t r28;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
	uint32_t cr;
	ipl_t ipl;
} context_t;

#endif
#endif
