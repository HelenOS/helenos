/*
 * SPDX-FileCopyrightText: 2009 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * SW implementation of 32 and 64 bit multiplication.
 */

#include <genarch/softint/multiplication.h>

/** Set 1 to return MAX_INT64 or MIN_INT64 on overflow */
#ifndef SOFTINT_CHECK_OF
# define SOFTINT_CHECK_OF 0
#endif

#define MAX_UINT16 (0xFFFFu)
#define MAX_UINT32 (0xFFFFFFFFu)
#define MAX_INT64 (0x7FFFFFFFFFFFFFFFll)
#define MIN_INT64 (0x8000000000000000ll)

/**
 * Multiply two integers and return long long as result.
 * This function is overflow safe.
 * @param a
 * @param b
 * @result
 */
static unsigned long long mul(unsigned int a, unsigned int b)
{
	unsigned int a1, a2, b1, b2;
	unsigned long long t1, t2, t3;

	a1 = a >> 16;
	a2 = a & MAX_UINT16;
	b1 = b >> 16;
	b2 = b & MAX_UINT16;

	t1 = a1 * b1;
	t2 = a1 * b2;
	t2 += a2 * b1;
	t3 = a2 * b2;

	t3 = (((t1 << 16) + t2) << 16) + t3;

	return t3;
}

/** Emulate multiplication of two 64-bit long long integers.
 *
 */
long long __muldi3 (long long a, long long b)
{
	long long result;
	unsigned long long t1, t2;
	unsigned long long a1, a2, b1, b2;
	char neg = 0;

	if (a < 0) {
		neg = !neg;
		a = -a;
	}

	if (b < 0) {
		neg = !neg;
		b = -b;
	}

	a1 = a >> 32;
	b1 = b >> 32;

	a2 = a & (MAX_UINT32);
	b2 = b & (MAX_UINT32);

	if (SOFTINT_CHECK_OF && (a1 != 0) && (b1 != 0)) {
		// error, overflow
		return (neg ? MIN_INT64 : MAX_INT64);
	}

	// (if OF checked) a1 or b1 is zero => result fits in 64 bits, no need to another overflow check
	t1 = mul(a1, b2) + mul(b1, a2);

	if (SOFTINT_CHECK_OF && t1 > MAX_UINT32) {
		// error, overflow
		return (neg ? MIN_INT64 : MAX_INT64);
	}

	t1 = t1 << 32;
	t2 = mul(a2, b2);
	t2 += t1;

	/*
	 * t2 & (1ull << 63) - if this bit is set in unsigned long long,
	 * result does not fit in signed one
	 */
	if (SOFTINT_CHECK_OF && ((t2 < t1) || (t2 & (1ull << 63)))) {
		// error, overflow
		return (neg ? MIN_INT64 : MAX_INT64);
	}

	result = t2;

	if (neg) {
		result = -result;
	}

	return result;
}

/** @}
 */
