/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
