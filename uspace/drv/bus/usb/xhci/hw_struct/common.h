/*
 * Copyright (c) 2018 Ondrej Hlavaty
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief Common macros for HW structures.
 *
 * There are a lot of data structures that are defined on bit-basis.
 * Therefore, we provide macros to define getters and setters.
 */

#ifndef XHCI_COMMON_H
#define XHCI_COMMON_H

#include <assert.h>
#include <bitops.h>
#include <byteorder.h>
#include <ddi.h>
#include <errno.h>

#define host2xhci(size, val) host2uint##size##_t_le((val))
#define xhci2host(size, val) uint##size##_t_le2host((val))

/**
 * 4 bytes, little-endian.
 */
typedef ioport32_t xhci_dword_t __attribute__((aligned(4)));

/**
 * 8 bytes, little-endian.
 */
typedef volatile uint64_t xhci_qword_t __attribute__((aligned(8)));

#define XHCI_DWORD_EXTRACT(field, hi, lo) \
	(BIT_RANGE_EXTRACT(uint32_t, hi, lo, xhci2host(32, field)))
#define XHCI_QWORD_EXTRACT(field, hi, lo) \
	(BIT_RANGE_EXTRACT(uint64_t, hi, lo, xhci2host(64, field)))

/**
 * Common base for setters on xhci_dword_t storage.
 *
 * Not thread-safe, proper synchronization over this dword must be assured.
 */
static inline void xhci_dword_set_bits(xhci_dword_t *storage, uint32_t value,
	unsigned hi, unsigned lo)
{
	const uint32_t mask = host2xhci(32, BIT_RANGE(uint32_t, hi, lo));
	const uint32_t set = host2xhci(32, value << lo);
	*storage = (*storage & ~mask) | set;
}

/**
 * Setter for whole qword.
 */
static inline void xhci_qword_set(xhci_qword_t *storage, uint64_t value)
{
	*storage = host2xhci(64, value);
}

static inline void xhci_qword_set_bits(xhci_qword_t *storage, uint64_t value,
	unsigned hi, unsigned lo)
{
	const uint64_t mask = host2xhci(64, BIT_RANGE(uint64_t, hi, lo));
	const uint64_t set = host2xhci(64, value << lo);
	*storage = (*storage & ~mask) | set;
}

static inline int xhci_reg_wait(xhci_dword_t *reg, uint32_t mask,
	uint32_t expected)
{
	mask = host2xhci(32, mask);
	expected = host2xhci(32, expected);

	unsigned retries = 100;
	uint32_t value = *reg & mask;

	for (; retries > 0 && value != expected; --retries) {
		async_usleep(10000);
		value = *reg & mask;
	}

	return value == expected ? EOK : ETIMEOUT;
}

#endif

/**
 * @}
 */
