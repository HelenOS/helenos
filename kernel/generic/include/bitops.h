/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
