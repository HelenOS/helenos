/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#include <genarch/softint/division.h>

#define ABSVAL(x)  ((x) > 0 ? (x) : -(x))
#define SGN(x)     ((x) >= 0 ? 1 : 0)

static unsigned int divandmod32(unsigned int a, unsigned int b,
    unsigned int *remainder)
{
	unsigned int result;
	int steps = sizeof(unsigned int) * 8;

	*remainder = 0;
	result = 0;

	if (b == 0) {
		/* FIXME: division by zero */
		return 0;
	}

	if (a < b) {
		*remainder = a;
		return 0;
	}

	for (; steps > 0; steps--) {
		/* shift one bit to remainder */
		*remainder = ((*remainder) << 1) | ((a >> 31) & 0x1);
		result <<= 1;

		if (*remainder >= b) {
			*remainder -= b;
			result |= 0x1;
		}
		a <<= 1;
	}

	return result;
}

static unsigned long long divandmod64(unsigned long long a,
    unsigned long long b, unsigned long long *remainder)
{
	unsigned long long result;
	int steps = sizeof(unsigned long long) * 8;

	*remainder = 0;
	result = 0;

	if (b == 0) {
		/* FIXME: division by zero */
		return 0;
	}

	if (a < b) {
		*remainder = a;
		return 0;
	}

	for (; steps > 0; steps--) {
		/* shift one bit to remainder */
		*remainder = ((*remainder) << 1) | ((a >> 63) & 0x1);
		result <<= 1;

		if (*remainder >= b) {
			*remainder -= b;
			result |= 0x1;
		}
		a <<= 1;
	}

	return result;
}

/* 32bit integer division */
int __divsi3(int a, int b)
{
	unsigned int rem;
	int result = (int) divandmod32(ABSVAL(a), ABSVAL(b), &rem);

	if (SGN(a) == SGN(b))
		return result;

	return -result;
}

/* 64bit integer division */
long long __divdi3(long long a, long long b)
{
	unsigned long long rem;
	long long result = (long long) divandmod64(ABSVAL(a), ABSVAL(b), &rem);

	if (SGN(a) == SGN(b))
		return result;

	return -result;
}

/* 32bit unsigned integer division */
unsigned int __udivsi3(unsigned int a, unsigned int b)
{
	unsigned int rem;
	return divandmod32(a, b, &rem);
}

/* 64bit unsigned integer division */
unsigned long long __udivdi3(unsigned long long a, unsigned long long b)
{
	unsigned long long rem;
	return divandmod64(a, b, &rem);
}

/* 32bit remainder of the signed division */
int __modsi3(int a, int b)
{
	unsigned int rem;
	divandmod32(a, b, &rem);

	/* if divident is negative, remainder must be too */
	if (!(SGN(a)))
		return -((int) rem);

	return (int) rem;
}

/* 64bit remainder of the signed division */
long long __moddi3(long long a, long long b)
{
	unsigned long long rem;
	divandmod64(a, b, &rem);

	/* if divident is negative, remainder must be too */
	if (!(SGN(a)))
		return -((long long) rem);

	return (long long) rem;
}

/* 32bit remainder of the unsigned division */
unsigned int __umodsi3(unsigned int a, unsigned int b)
{
	unsigned int rem;
	divandmod32(a, b, &rem);
	return rem;
}

/* 64bit remainder of the unsigned division */
unsigned long long __umoddi3(unsigned long long a, unsigned long long b)
{
	unsigned long long rem;
	divandmod64(a, b, &rem);
	return rem;
}

int __divmodsi3(int a, int b, int *c)
{
	unsigned int rem;
	int result = (int) divandmod32(ABSVAL(a), ABSVAL(b), &rem);

	if (SGN(a) == SGN(b)) {
		*c = rem;
		return result;
	}

	*c = -rem;
	return -result;
}

unsigned int __udivmodsi3(unsigned int a, unsigned int b,
    unsigned int *c)
{
	return divandmod32(a, b, c);
}

long long __divmoddi3(long long a, long long b, long long *c)
{
	unsigned long long rem;
	long long result = (int) divandmod64(ABSVAL(a), ABSVAL(b), &rem);

	if (SGN(a) == SGN(b)) {
		*c = rem;
		return result;
	}

	*c = -rem;
	return -result;
}

unsigned long long __udivmoddi3(unsigned long long a, unsigned long long b,
    unsigned long long *c)
{
	return divandmod64(a, b, c);
}

unsigned long long __udivmoddi4(unsigned long long a, unsigned long long b,
    unsigned long long *c)
{
	return divandmod64(a, b, c);
}

/** @}
 */
