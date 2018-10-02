/*
 * Copyright (c) 2005 Jakub Jermar
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

#ifndef KERN_BYTEORDER_H_
#define KERN_BYTEORDER_H_

#include <stdint.h>

#if !(defined(__BE__) ^ defined(__LE__))
#error The architecture must be either big-endian or little-endian.
#endif

#ifdef __BE__

#define uint16_t_le2host(n)  (uint16_t_byteorder_swap(n))
#define uint32_t_le2host(n)  (uint32_t_byteorder_swap(n))
#define uint64_t_le2host(n)  (uint64_t_byteorder_swap(n))

#define uint16_t_be2host(n)  (n)
#define uint32_t_be2host(n)  (n)
#define uint64_t_be2host(n)  (n)

#define host2uint16_t_le(n)  (uint16_t_byteorder_swap(n))
#define host2uint32_t_le(n)  (uint32_t_byteorder_swap(n))
#define host2uint64_t_le(n)  (uint64_t_byteorder_swap(n))

#define host2uint16_t_be(n)  (n)
#define host2uint32_t_be(n)  (n)
#define host2uint64_t_be(n)  (n)

#else

#define uint16_t_le2host(n)  (n)
#define uint32_t_le2host(n)  (n)
#define uint64_t_le2host(n)  (n)

#define uint16_t_be2host(n)  (uint16_t_byteorder_swap(n))
#define uint32_t_be2host(n)  (uint32_t_byteorder_swap(n))
#define uint64_t_be2host(n)  (uint64_t_byteorder_swap(n))

#define host2uint16_t_le(n)  (n)
#define host2uint32_t_le(n)  (n)
#define host2uint64_t_le(n)  (n)

#define host2uint16_t_be(n)  (uint16_t_byteorder_swap(n))
#define host2uint32_t_be(n)  (uint32_t_byteorder_swap(n))
#define host2uint64_t_be(n)  (uint64_t_byteorder_swap(n))

#endif

static inline uint64_t uint64_t_byteorder_swap(uint64_t n)
{
	return ((n & 0xff) << 56) |
	    ((n & 0xff00) << 40) |
	    ((n & 0xff0000) << 24) |
	    ((n & 0xff000000LL) << 8) |
	    ((n & 0xff00000000LL) >> 8) |
	    ((n & 0xff0000000000LL) >> 24) |
	    ((n & 0xff000000000000LL) >> 40) |
	    ((n & 0xff00000000000000LL) >> 56);
}

static inline uint32_t uint32_t_byteorder_swap(uint32_t n)
{
	return ((n & 0xff) << 24) |
	    ((n & 0xff00) << 8) |
	    ((n & 0xff0000) >> 8) |
	    ((n & 0xff000000) >> 24);
}

static inline uint16_t uint16_t_byteorder_swap(uint16_t n)
{
	return ((n & 0xff) << 8) |
	    ((n & 0xff00) >> 8);
}

#endif

/** @}
 */
