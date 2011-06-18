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
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "../internal/common.h"

#include "../libc/assert.h"
#include "../ctype.h"
#include <errno.h> // TODO: use POSIX errno
#include "../libc/bool.h"
#include "../libc/stdint.h"
#include "../stdlib.h"
#include "../strings.h"

#ifndef HUGE_VALL
	#define HUGE_VALL (+1.0l / +0.0l)
#endif

#ifndef abs
	#define abs(x) ((x < 0) ? -x : x)
#endif

// TODO: documentation

// FIXME: ensure it builds and works on all platforms

const int max_small_pow5 = 15;

/* The value at index i is approximately 5**i. */
long double small_pow5[] = {
	0x1P0,
	0x5P0,
	0x19P0,
	0x7dP0,
	0x271P0,
	0xc35P0,
	0x3d09P0,
	0x1312dP0,
	0x5f5e1P0,
	0x1dcd65P0,
	0x9502f9P0,
	0x2e90eddP0,
	0xe8d4a51P0,
	0x48c27395P0,
	0x16bcc41e9P0,
	0x71afd498dP0
};

/* The value at index i is approximately 5**(2**i). */
long double large_pow5[] = {
	0x5P0l,
	0x19P0l,
	0x271P0l,
	0x5f5e1P0l,
	0x2386f26fc1P0l,
	0x4ee2d6d415b85acef81P0l,
	0x184f03e93ff9f4daa797ed6e38ed64bf6a1f01P0l,
	0x24ee91f2603a6337f19bccdb0dac404dc08d3cff5ecP128l,
	0x553f75fdcefcef46eeddcP512l,
	0x1c633415d4c1d238d98cab8a978a0b1f138cb07303P1024l,
	0x325d9d61a05d4305d9434f4a3c62d433949ae6209d492P2200l,
	0x9e8b3b5dc53d5de4a74d28ce329ace526a3197bbebe3034f77154ce2bcba1964P4500l,
	0x6230290145104bcd64a60a9fc025254932bb0fd922271133eeae7P9300l
};

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
	0x1P1024l,
	0x1P2048l,
	0x1P4096l,
	0x1P8192l
};

static inline bool out_of_range(long double num)
{
	return num == 0.0l || num == HUGE_VALL;
}

/**
 * Multiplies a number by a power of five.
 * The result is not exact and may not be the best possible approximation.
 *
 * @param base Number to be multiplied.
 * @param exponent Base 5 exponent.
 * @return base multiplied by 5**exponent
 */
static long double mul_pow5(long double base, int exponent)
{
	if (out_of_range(base)) {
		return base;
	}
	
	if (abs(exponent) >> 13 != 0) {
		errno = ERANGE;
		return exponent < 0 ? 0.0l : HUGE_VALL;
	}
	
	if (exponent < 0) {
		exponent = -exponent;
		base /= small_pow5[exponent & 0xF];
		for (int i = 4; i < 13; ++i) {
			if (((exponent >> i) & 1) != 0) {
				base /= large_pow5[i];
				if (out_of_range(base)) {
					errno = ERANGE;
					break;
				}
			}
		}
	} else {
		base *= small_pow5[exponent & 0xF];
		for (int i = 4; i < 13; ++i) {
			if (((exponent >> i) & 1) != 0) {
				base *= large_pow5[i];
				if (out_of_range(base)) {
					errno = ERANGE;
					break;
				}
			}
		}
	}
	
	return base;
}

/**
 * Multiplies a number by a power of two.
 *
 * @param base Number to be multiplied.
 * @param exponent Base 2 exponent.
 * @return base multiplied by 2**exponent
 */
static long double mul_pow2(long double base, int exponent)
{
	if (out_of_range(base)) {
		return base;
	}
	
	if (abs(exponent) >> 14 != 0) {
		errno = ERANGE;
		return exponent < 0 ? 0.0l : HUGE_VALL;
	}
	
	if (exponent < 0) {
		exponent = -exponent;
		for (int i = 0; i < 14; ++i) {
			if (((exponent >> i) & 1) != 0) {
				base /= pow2[i];
				if (out_of_range(base)) {
					errno = ERANGE;
					break;
				}
			}
		}
	} else {
		for (int i = 0; i < 14; ++i) {
			if (((exponent >> i) & 1) != 0) {
				base *= pow2[i];
				if (out_of_range(base)) {
					errno = ERANGE;
					break;
				}
			}
		}
	}
	
	return base;
}


static long double parse_decimal(const char **sptr)
{
	// TODO: Use strtol(), at least for exponent.
	
	const int DEC_BASE = 10;
	const char DECIMAL_POINT = '.';
	const char EXPONENT_MARK = 'e';
	/* The highest amount of digits that can be safely parsed
	 * before an overflow occurs.
	 */
	const int PARSE_DECIMAL_DIGS = 19;
	
	/* significand */
	uint64_t significand = 0;
	
	/* position in the input string */
	int i = 0;
	
	/* number of digits parsed so far */
	int parsed_digits = 0;
	
	int exponent = 0;
	
	const char *str = *sptr;
	
	/* digits before decimal point */
	while (isdigit(str[i])) {
		if (parsed_digits < PARSE_DECIMAL_DIGS) {
			significand *= DEC_BASE;
			significand += str[i] - '0';
			parsed_digits++;
		} else {
			exponent++;
		}
		
		i++;
	}
	
	if (str[i] == DECIMAL_POINT) {
		i++;
		
		/* digits after decimal point */
		while (isdigit(str[i])) {
			if (parsed_digits < PARSE_DECIMAL_DIGS) {
				significand *= DEC_BASE;
				significand += str[i] - '0';
				exponent--;
				parsed_digits++;
			} else {
				/* ignore */
			}
			
			i++;
		}
	}
	
	/* exponent */
	if (tolower(str[i]) == EXPONENT_MARK) {
		i++;
		
		bool negative = false;
		int exp = 0;
		
		switch (str[i]) {
		case '-':
			negative = true;
			/* fallthrough */
		case '+':
			i++;
		}
		
		while (isdigit(str[i])) {
			exp *= DEC_BASE;
			exp += str[i] - '0';
			
			i++;
		}
		
		if (negative) {
			exp = -exp;
		}
		
		exponent += exp;
	}
	
	long double result = (long double) significand;
	result = mul_pow5(result, exponent);
	if (result != HUGE_VALL) {
		result = mul_pow2(result, exponent);
	}
	
	*sptr = &str[i];
	return result;
}

static inline int hex_value(char ch) {
	if (ch <= '9') {
		return ch - '0';
	} else {
		return 10 + tolower(ch) - 'a';
	}
}

static long double parse_hexadecimal(const char **sptr)
{
	// TODO: Use strtol(), at least for exponent.
	
	/* this function currently always rounds to zero */
	// TODO: honor rounding mode
	
	const int DEC_BASE = 10;
	const int HEX_BASE = 16;
	const char DECIMAL_POINT = '.';
	const char EXPONENT_MARK = 'p';
	/* The highest amount of digits that can be safely parsed
	 * before an overflow occurs.
	 */
	const int PARSE_HEX_DIGS = 16;
	
	/* significand */
	uint64_t significand = 0;
	
	/* position in the input string */
	int i = 0;
	
	/* number of digits parsed so far */
	int parsed_digits = 0;
	
	int exponent = 0;
	
	const char *str = *sptr;
	
	/* digits before decimal point */
	while (posix_isxdigit(str[i])) {
		if (parsed_digits < PARSE_HEX_DIGS) {
			significand *= HEX_BASE;
			significand += hex_value(str[i]);
			parsed_digits++;
		} else {
			exponent += 4;
		}
		
		i++;
	}
	
	if (str[i] == DECIMAL_POINT) {
		i++;
		
		/* digits after decimal point */
		while (posix_isxdigit(str[i])) {
			if (parsed_digits < PARSE_HEX_DIGS) {
				significand *= HEX_BASE;
				significand += hex_value(str[i]);
				exponent -= 4;
				parsed_digits++;
			} else {
				/* ignore */
			}
			
			i++;
		}
	}
	
	/* exponent */
	if (tolower(str[i]) == EXPONENT_MARK) {
		i++;
		
		bool negative = false;
		int exp = 0;
		
		switch (str[i]) {
		case '-':
			negative = true;
			/* fallthrough */
		case '+':
			i++;
		}
		
		while (isdigit(str[i])) {
			exp *= DEC_BASE;
			exp += str[i] - '0';
			
			i++;
		}
		
		if (negative) {
			exp = -exp;
		}
		
		exponent += exp;
	}
	
	long double result = (long double) significand;
	result = mul_pow2(result, exponent);
	
	*sptr = &str[i];
	return result;
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
 *    unrecognized character.
 * @return An approximate representation of the input floating-point number.
 */
long double posix_strtold(const char *restrict nptr, char **restrict endptr)
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
		/* fallthrough */
	case '+':
		i++;
	}
	
	/* check for NaN */
	if (posix_strncasecmp(&nptr[i], "nan", 3) == 0) {
		// FIXME: return NaN
		// TODO: handle the parenthesised case
		
		if (endptr != NULL) {
			*endptr = (char *) &nptr[i + 3];
		}
		errno = ERANGE;
		return negative ? -0.0l : +0.0l;
	}
	
	/* check for Infinity */
	if (posix_strncasecmp(&nptr[i], "inf", 3) == 0) {
		i += 3;
		if (posix_strncasecmp(&nptr[i], "inity", 5) == 0) {
			i += 5;
		}
		
		if (endptr != NULL) {
			*endptr = (char *) &nptr[i];
		}
		return negative ? -HUGE_VALL : +HUGE_VALL;
	}

	/* check for a hex number */
	if (nptr[i] == '0' && tolower(nptr[i + 1]) == 'x' &&
	    (posix_isxdigit(nptr[i + 2]) ||
	    (nptr[i + 2] == RADIX && posix_isxdigit(nptr[i + 3])))) {
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

