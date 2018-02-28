/*
 * Copyright (c) 2011 Jiri Zarevucky
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

/** @addtogroup libposix
 * @{
 */
/** @file Backend for floating point conversions.
 */

#include "../internal/common.h"
#include "libc/stdbool.h"
#include "posix/stdlib.h"

#include <assert.h>
#include "posix/ctype.h"
#include "posix/stdint.h"
#include "posix/strings.h"

#include <errno.h>

#include "posix/limits.h"

#include "posix/float.h"

#include "libc/str.h"

#ifndef HUGE_VALL
	#define HUGE_VALL (+1.0l / +0.0l)
#endif

#ifndef abs
	#define abs(x) (((x) < 0) ? -(x) : (x))
#endif

/* If the constants are not defined, use double precision as default. */
#ifndef LDBL_MANT_DIG
	#define LDBL_MANT_DIG 53
#endif
#ifndef LDBL_MAX_EXP
	#define LDBL_MAX_EXP 1024
#endif
#ifndef LDBL_MIN_EXP
	#define LDBL_MIN_EXP (-1021)
#endif
#ifndef LDBL_DIG
	#define LDBL_DIG 15
#endif
#ifndef LDBL_MIN
	#define LDBL_MIN 2.2250738585072014E-308
#endif

/* power functions ************************************************************/

#if LDBL_MAX_EXP >= 16384
const int MAX_POW5 = 12;
#else
const int MAX_POW5 = 8;
#endif

/* The value at index i is approximately 5**(2**i). */
long double pow5[] = {
	0x5p0l,
	0x19p0l,
	0x271p0l,
	0x5F5E1p0l,
	0x2386F26FC1p0l,
	0x4EE2D6D415B85ACEF81p0l,
	0x184F03E93FF9F4DAA797ED6E38ED6p36l,
	0x127748F9301D319BF8CDE66D86D62p185l,
	0x154FDD7F73BF3BD1BBB77203731FDp482l,
#if LDBL_MAX_EXP >= 16384
	0x1C633415D4C1D238D98CAB8A978A0p1076l,
	0x192ECEB0D02EA182ECA1A7A51E316p2265l,
	0x13D1676BB8A7ABBC94E9A519C6535p4643l,
	0x188C0A40514412F3592982A7F0094p9398l,
#endif
};

#if LDBL_MAX_EXP >= 16384
const int MAX_POW2 = 15;
#else
const int MAX_POW2 = 9;
#endif

/* Powers of two. */
long double pow2[] = {
	0x1P1l,
	0x1P2l,
	0x1P4l,
	0x1P8l,
	0x1P16l,
	0x1P32l,
	0x1P64l,
	0x1P128l,
	0x1P256l,
	0x1P512l,
#if LDBL_MAX_EXP >= 16384
	0x1P1024l,
	0x1P2048l,
	0x1P4096l,
	0x1P8192l,
#endif
};

/**
 * Multiplies a number by a power of five.
 * The result may be inexact and may not be the best possible approximation.
 *
 * @param mant Number to be multiplied.
 * @param exp Base 5 exponent.
 * @return mant multiplied by 5**exp
 */
static long double mul_pow5(long double mant, int exp)
{
	if (mant == 0.0l || mant == HUGE_VALL) {
		return mant;
	}

	if (abs(exp) >> (MAX_POW5 + 1) != 0) {
		/* Too large exponent. */
		errno = ERANGE;
		return exp < 0 ? LDBL_MIN : HUGE_VALL;
	}

	if (exp < 0) {
		exp = abs(exp);
		for (int bit = 0; bit <= MAX_POW5; ++bit) {
			/* Multiply by powers of five bit-by-bit. */
			if (((exp >> bit) & 1) != 0) {
				mant /= pow5[bit];
				if (mant == 0.0l) {
					/* Underflow. */
					mant = LDBL_MIN;
					errno = ERANGE;
					break;
				}
			}
		}
	} else {
		for (int bit = 0; bit <= MAX_POW5; ++bit) {
			/* Multiply by powers of five bit-by-bit. */
			if (((exp >> bit) & 1) != 0) {
				mant *= pow5[bit];
				if (mant == HUGE_VALL) {
					/* Overflow. */
					errno = ERANGE;
					break;
				}
			}
		}
	}

	return mant;
}

/**
 * Multiplies a number by a power of two. This is always exact.
 *
 * @param mant Number to be multiplied.
 * @param exp Base 2 exponent.
 * @return mant multiplied by 2**exp.
 */
static long double mul_pow2(long double mant, int exp)
{
	if (mant == 0.0l || mant == HUGE_VALL) {
		return mant;
	}

	if (exp > LDBL_MAX_EXP || exp < LDBL_MIN_EXP) {
		errno = ERANGE;
		return exp < 0 ? LDBL_MIN : HUGE_VALL;
	}

	if (exp < 0) {
		exp = abs(exp);
		for (int i = 0; i <= MAX_POW2; ++i) {
			if (((exp >> i) & 1) != 0) {
				mant /= pow2[i];
				if (mant == 0.0l) {
					mant = LDBL_MIN;
					errno = ERANGE;
					break;
				}
			}
		}
	} else {
		for (int i = 0; i <= MAX_POW2; ++i) {
			if (((exp >> i) & 1) != 0) {
				mant *= pow2[i];
				if (mant == HUGE_VALL) {
					errno = ERANGE;
					break;
				}
			}
		}
	}

	return mant;
}

/* end power functions ********************************************************/



/**
 * Convert decimal string representation of the floating point number.
 * Function expects the string pointer to be already pointed at the first
 * digit (i.e. leading optional sign was already consumed by the caller).
 *
 * @param sptr Pointer to the storage of the string pointer. Upon successful
 *     conversion, the string pointer is updated to point to the first
 *     unrecognized character.
 * @return An approximate representation of the input floating-point number.
 */
static long double parse_decimal(const char **sptr)
{
	assert(sptr != NULL);
	assert (*sptr != NULL);

	const int DEC_BASE = 10;
	const char DECIMAL_POINT = '.';
	const char EXPONENT_MARK = 'e';

	const char *str = *sptr;
	long double significand = 0;
	long exponent = 0;

	/* number of digits parsed so far */
	int parsed_digits = 0;
	bool after_decimal = false;

	while (isdigit(*str) || (!after_decimal && *str == DECIMAL_POINT)) {
		if (*str == DECIMAL_POINT) {
			after_decimal = true;
			str++;
			continue;
		}

		if (parsed_digits == 0 && *str == '0') {
			/* Nothing, just skip leading zeros. */
		} else if (parsed_digits < LDBL_DIG) {
			significand = significand * DEC_BASE + (*str - '0');
			parsed_digits++;
		} else {
			exponent++;
		}

		if (after_decimal) {
			/* Decrement exponent if we are parsing the fractional part. */
			exponent--;
		}

		str++;
	}

	/* exponent */
	if (tolower(*str) == EXPONENT_MARK) {
		str++;

		/* Returns MIN/MAX value on error, which is ok. */
		long exp = strtol(str, (char **) &str, DEC_BASE);

		if (exponent > 0 && exp > LONG_MAX - exponent) {
			exponent = LONG_MAX;
		} else if (exponent < 0 && exp < LONG_MIN - exponent) {
			exponent = LONG_MIN;
		} else {
			exponent += exp;
		}
	}

	*sptr = str;

	/* Return multiplied by a power of ten. */
	return mul_pow2(mul_pow5(significand, exponent), exponent);
}

/**
 * Derive a hexadecimal digit from its character representation.
 *
 * @param ch Character representation of the hexadecimal digit.
 * @return Digit value represented by an integer.
 */
static inline int hex_value(char ch)
{
	if (ch <= '9') {
		return ch - '0';
	} else {
		return 10 + tolower(ch) - 'a';
	}
}

/**
 * Convert hexadecimal string representation of the floating point number.
 * Function expects the string pointer to be already pointed at the first
 * digit (i.e. leading optional sign and 0x prefix were already consumed
 * by the caller).
 *
 * @param sptr Pointer to the storage of the string pointer. Upon successful
 *     conversion, the string pointer is updated to point to the first
 *     unrecognized character.
 * @return Representation of the input floating-point number.
 */
static long double parse_hexadecimal(const char **sptr)
{
	assert(sptr != NULL && *sptr != NULL);

	const int DEC_BASE = 10;
	const int HEX_BASE = 16;
	const char DECIMAL_POINT = '.';
	const char EXPONENT_MARK = 'p';

	const char *str = *sptr;
	long double significand = 0;
	long exponent = 0;

	/* number of bits parsed so far */
	int parsed_bits = 0;
	bool after_decimal = false;

	while (isxdigit(*str) || (!after_decimal && *str == DECIMAL_POINT)) {
		if (*str == DECIMAL_POINT) {
			after_decimal = true;
			str++;
			continue;
		}

		if (parsed_bits == 0 && *str == '0') {
			/* Nothing, just skip leading zeros. */
		} else if (parsed_bits <= LDBL_MANT_DIG) {
			significand = significand * HEX_BASE + hex_value(*str);
			parsed_bits += 4;
		} else {
			exponent += 4;
		}

		if (after_decimal) {
			exponent -= 4;
		}

		str++;
	}

	/* exponent */
	if (tolower(*str) == EXPONENT_MARK) {
		str++;

		/* Returns MIN/MAX value on error, which is ok. */
		long exp = strtol(str, (char **) &str, DEC_BASE);

		if (exponent > 0 && exp > LONG_MAX - exponent) {
			exponent = LONG_MAX;
		} else if (exponent < 0 && exp < LONG_MIN - exponent) {
			exponent = LONG_MIN;
		} else {
			exponent += exp;
		}
	}

	*sptr = str;

	/* Return multiplied by a power of two. */
	return mul_pow2(significand, exponent);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. Largely POSIX compliant, except for
 * locale differences (always uses '.' at the moment) and rounding.
 * Decimal strings are NOT guaranteed to be correctly rounded. This function
 * should return a good enough approximation for most purposes but if you
 * depend on a precise conversion, use hexadecimal representation.
 * Hexadecimal strings are currently always rounded towards zero, regardless
 * of the current rounding mode.
 *
 * @param nptr Input string.
 * @param endptr If non-NULL, *endptr is set to the position of the first
 *     unrecognized character.
 * @return An approximate representation of the input floating-point number.
 */
long double strtold(const char *restrict nptr, char **restrict endptr)
{
	assert(nptr != NULL);

	const int RADIX = '.';

	/* minus sign */
	bool negative = false;
	/* current position in the string */
	int i = 0;

	/* skip whitespace */
	while (isspace(nptr[i])) {
		i++;
	}

	/* parse sign */
	switch (nptr[i]) {
	case '-':
		negative = true;
		/* Fallthrough */
	case '+':
		i++;
	}

	/* check for NaN */
	if (strncasecmp(&nptr[i], "nan", 3) == 0) {
		// FIXME: return NaN
		// TODO: handle the parenthesised case

		if (endptr != NULL) {
			*endptr = (char *) nptr;
		}
		errno = EINVAL;
		return 0;
	}

	/* check for Infinity */
	if (strncasecmp(&nptr[i], "inf", 3) == 0) {
		i += 3;
		if (strncasecmp(&nptr[i], "inity", 5) == 0) {
			i += 5;
		}

		if (endptr != NULL) {
			*endptr = (char *) &nptr[i];
		}
		return negative ? -HUGE_VALL : +HUGE_VALL;
	}

	/* check for a hex number */
	if (nptr[i] == '0' && tolower(nptr[i + 1]) == 'x' &&
	    (isxdigit(nptr[i + 2]) ||
	    (nptr[i + 2] == RADIX && isxdigit(nptr[i + 3])))) {
		i += 2;

		const char *ptr = &nptr[i];
		/* this call sets errno if appropriate. */
		long double result = parse_hexadecimal(&ptr);
		if (endptr != NULL) {
			*endptr = (char *) ptr;
		}
		return negative ? -result : result;
	}

	/* check for a decimal number */
	if (isdigit(nptr[i]) || (nptr[i] == RADIX && isdigit(nptr[i + 1]))) {
		const char *ptr = &nptr[i];
		/* this call sets errno if appropriate. */
		long double result = parse_decimal(&ptr);
		if (endptr != NULL) {
			*endptr = (char *) ptr;
		}
		return negative ? -result : result;
	}

	/* nothing to parse */
	if (endptr != NULL) {
		*endptr = (char *) nptr;
	}
	errno = EINVAL;
	return 0;
}

/** @}
 */
