/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BIGINT_T_H_
#define BIGINT_T_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t bigint_word_t;
typedef uint16_t bigint_dword_t;

#define BIGINT_BASE ((bigint_dword_t) 256UL)

/** Big integer.
 *
 * Used to implement Sysel @c int type.
 */
typedef struct bigint {
	/** Number of non-zero digits in the @c digit array. */
	size_t length;

	/** Sign. */
	bool negative;

	/** Digits starting from the least significant. */
	bigint_word_t *digit;
} bigint_t;

#endif
