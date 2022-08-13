/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_ARCH_FIBRIL_CONTEXT_H_
#define _LIBC_ARCH_FIBRIL_CONTEXT_H_

#define __CONTEXT_OFFSET_SP   0x00
#define __CONTEXT_OFFSET_PC   0x04
#define __CONTEXT_OFFSET_EBX  0x08
#define __CONTEXT_OFFSET_ESI  0x0c
#define __CONTEXT_OFFSET_EDI  0x10
#define __CONTEXT_OFFSET_EBP  0x14
#define __CONTEXT_OFFSET_TLS  0x18
#define __CONTEXT_SIZE        0x1c

#ifndef __ASSEMBLER__

#include <stdint.h>

/* We include only registers that must be preserved during function call. */
typedef struct __context {
	uint32_t sp;
	uint32_t pc;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t tls;
} __context_t;

#endif
#endif
