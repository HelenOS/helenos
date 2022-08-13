/*
 * SPDX-FileCopyrightText: 2010 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_abs32le_FIBRIL_CONTEXT_H_
#define _LIBC_abs32le_FIBRIL_CONTEXT_H_

#include <stdint.h>

/*
 * On real hardware this stores the registers which
 * need to be preserved across function calls.
 */
typedef struct __context {
	uintptr_t sp;
	uintptr_t fp;
	uintptr_t pc;
	uintptr_t tls;
} __context_t;

#endif
