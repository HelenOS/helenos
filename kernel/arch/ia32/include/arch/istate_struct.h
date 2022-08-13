/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_EDX 0
#define ISTATE_SIZE_EDX 4
#define ISTATE_OFFSET_ECX 4
#define ISTATE_SIZE_ECX 4
#define ISTATE_OFFSET_EBX 8
#define ISTATE_SIZE_EBX 4
#define ISTATE_OFFSET_ESI 12
#define ISTATE_SIZE_ESI 4
#define ISTATE_OFFSET_EDI 16
#define ISTATE_SIZE_EDI 4
#define ISTATE_OFFSET_EBP 20
#define ISTATE_SIZE_EBP 4
#define ISTATE_OFFSET_EAX 24
#define ISTATE_SIZE_EAX 4
#define ISTATE_OFFSET_EBP_FRAME 28
#define ISTATE_SIZE_EBP_FRAME 4
#define ISTATE_OFFSET_EIP_FRAME 32
#define ISTATE_SIZE_EIP_FRAME 4
#define ISTATE_OFFSET_GS 36
#define ISTATE_SIZE_GS 4
#define ISTATE_OFFSET_FS 40
#define ISTATE_SIZE_FS 4
#define ISTATE_OFFSET_ES 44
#define ISTATE_SIZE_ES 4
#define ISTATE_OFFSET_DS 48
#define ISTATE_SIZE_DS 4
#define ISTATE_OFFSET_ERROR_WORD 52
#define ISTATE_SIZE_ERROR_WORD 4
#define ISTATE_OFFSET_EIP 56
#define ISTATE_SIZE_EIP 4
#define ISTATE_OFFSET_CS 60
#define ISTATE_SIZE_CS 4
#define ISTATE_OFFSET_EFLAGS 64
#define ISTATE_SIZE_EFLAGS 4
#define ISTATE_OFFSET_ESP 68
#define ISTATE_SIZE_ESP 4
#define ISTATE_OFFSET_SS 72
#define ISTATE_SIZE_SS 4
#define ISTATE_SIZE 76

#ifndef __ASSEMBLER__

#include <stdint.h>

/*
 * The strange order of the GPRs is given by the requirement to use the
 * istate structure for both regular interrupts and exceptions as well
 * as for syscall handlers which use this order as an optimization.
 */
typedef struct istate {
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t eax;
	/* imitation of frame pointer linkage */
	uint32_t ebp_frame;
	/* imitation of return address linkage */
	uint32_t eip_frame;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	/* real or fake error word */
	uint32_t error_word;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	/* only if istate_t is from uspace */
	uint32_t esp;
	/* only if istate_t is from uspace */
	uint32_t ss;
} istate_t;

#endif
#endif
