/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_CONTEXT_H_
#define KERN_amd64_CONTEXT_H_

#include <arch/context_struct.h>

/*
 * According to ABI the stack MUST be aligned on
 * 16-byte boundary. If it is not, the va_arg calling will
 * panic sooner or later
 */
#define SP_DELTA  16

#define context_set(c, _pc, stack, size) \
	do { \
		(c)->pc = (uintptr_t) (_pc); \
		(c)->sp = ((uintptr_t) (stack)) + (size) - SP_DELTA; \
		(c)->rbp = 0; \
	} while (0)

#endif

/** @}
 */
