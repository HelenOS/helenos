/*
 * SPDX-FileCopyrightText: 2014 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_ARCH_FIBRIL_CONTEXT_H_
#define _LIBC_ARCH_FIBRIL_CONTEXT_H_

#include <stdint.h>

/*
 * Only registers preserved across function calls are included. r9 is
 * used to store a TLS address. -ffixed-r9 gcc forces gcc not to use this
 * register. -mtp=soft forces gcc to use #__aeabi_read_tp to obtain
 * TLS address.
 */

// XXX: This struct must match the assembly code in src/fibril.S

typedef struct __context {
	uintptr_t sp;
	uintptr_t pc;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	/* r9 */
	uint32_t tls;
	uint32_t r10;
	/* r11 */
	uint32_t fp;
} __context_t;

#endif
