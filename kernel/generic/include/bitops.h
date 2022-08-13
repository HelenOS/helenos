/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_BITOPS_H_
#define KERN_BITOPS_H_

#include <stdint.h>
#include <trace.h>

#ifdef __32_BITS__
#define fnzb(arg)  fnzb32(arg)
#endif

#ifdef __64_BITS__
#define fnzb(arg)  fnzb64(arg)
#endif

/** Return position of first non-zero bit from left (32b variant).
 *
 * @return 0 (if the number is zero) or [log_2(arg)].
 *
 */
_NO_TRACE static inline uint8_t fnzb32(uint32_t arg)
{
	uint8_t n = 0;

	if (arg >> 16) {
		arg >>= 16;
		n += 16;
	}

	if (arg >> 8) {
		arg >>= 8;
		n += 8;
	}

	if (arg >> 4) {
		arg >>= 4;
		n += 4;
	}

	if (arg >> 2) {
		arg >>= 2;
		n += 2;
	}

	if (arg >> 1)
		n += 1;

	return n;
}

/** Return position of first non-zero bit from left (64b variant).
 *
 * @return 0 (if the number is zero) or [log_2(arg)].
 *
 */
_NO_TRACE static inline uint8_t fnzb64(uint64_t arg)
{
	uint8_t n = 0;

	if (arg >> 32) {
		arg >>= 32;
		n += 32;
	}

	return n + fnzb32((uint32_t) arg);
}

#endif

/** @}
 */
