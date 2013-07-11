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
/** @file Backend for integer conversions.
 */

#define LIBPOSIX_INTERNAL
#define __POSIX_DEF__(x) posix_##x

#include "../internal/common.h"
#include "posix/stdlib.h"

#include "posix/ctype.h"
#include "posix/errno.h"
#include "posix/inttypes.h"
#include "posix/limits.h"

#define intmax_t posix_intmax_t
#define uintmax_t posix_uintmax_t

/**
 * Decides whether a digit belongs to a particular base.
 *
 * @param c Character representation of the digit.
 * @param base Base against which the digit shall be tested.
 * @return True if the digit belongs to the base, false otherwise.
 */
static inline bool is_digit_in_base(int c, int base)
{
	if (base <= 10) {
		return c >= '0' && c < '0' + base;
	} else {
		return isdigit(c) ||
		    (tolower(c) >= 'a' && tolower(c) < ('a' + base - 10));
	}
}

/**
 * Derive a digit from its character representation.
 *
 * @param c Character representation of the digit.
 * @return Digit value represented by an integer.
 */
static inline int digit_value(int c)
{
	if (c <= '9') {
		return c - '0';
	} else {
		return 10 + tolower(c) - 'a';
	}
}

/**
 * Generic function for parsing an integer from it's string representation.
 * Different variants differ in lower and upper bounds.
 * The parsed string returned by this function is always positive, sign
 * information is provided via a dedicated parameter.
 *
 * @param nptr Input string.
 * @param endptr If non-NULL, *endptr is set to the position of the first
 *     unrecognized character. If no digit has been parsed, value of
 *     nptr is stored there (regardless of any skipped characters at the
 *     beginning).
 * @param base Expected base of the string representation. If 0, base is
 *    determined to be decimal, octal or hexadecimal using the same rules
 *    as C syntax. Otherwise, value must be between 2 and 36, inclusive.
 * @param min_value Lower bound for the resulting conversion.
 * @param max_value Upper bound for the resulting conversion.
 * @param out_negative Either NULL for unsigned conversion or a pointer to the
 *     bool variable into which shall be placed the negativity of the resulting
 *     converted value.
 * @return The absolute value of the parsed value, or the closest in-range value
 *     if the parsed value is out of range. If the input is invalid, zero is
 *     returned and errno is set to EINVAL.
 */
static inline uintmax_t internal_strtol(
    const char *restrict nptr, char **restrict endptr, int base,
    const intmax_t min_value, const uintmax_t max_value,
    bool *restrict out_negative)
{
	if (nptr == NULL) {
		errno = EINVAL;
		return 0;
	}
	
	if (base < 0 || base == 1 || base > 36) {
		errno = EINVAL;
		return 0;
	}
	
	/* The maximal absolute value that can be returned in this run.
	 * Depends on sign.
	 */
	uintmax_t real_max_value = max_value;
	
	/* Current index in the input string. */
	size_t i = 0;
	bool negative = false;
	
	/* Skip whitespace. */
	while (isspace(nptr[i])) {
		i++;
	}
	
	/* Parse sign. */
	switch (nptr[i]) {
	case '-':
		negative = true;
		
		/* The strange computation is are there to avoid a corner case
		 * where -min_value can't be represented in intmax_t.
		 * (I'm not exactly sure what the semantics are in such a
		 *  case, but this should be safe for any case.)
		 */
		real_max_value = (min_value == 0)
		    ? 0
		    :(((uintmax_t) -(min_value + 1)) + 1);
		
		/* fallthrough */
	case '+':
		i++;
	}
	
	/* Figure out the base. */
	switch (base) {
	case 0:
		if (nptr[i] == '0') {
			if (tolower(nptr[i + 1]) == 'x') {
				/* 0x... is hex. */
				base = 16;
				i += 2;
			} else {
				/* 0... is octal. */
				base = 8;
			}
		} else {
			/* Anything else is decimal by default. */
			base = 10;
		}
		break;
	case 16:
		/* Allow hex number to be prefixed with "0x". */
		if (nptr[i] == '0' && tolower(nptr[i + 1]) == 'x') {
			i += 2;
		}
		break;
	}
	
	if (!is_digit_in_base(nptr[i], base)) {
		/* No digits to parse, invalid input. */
		
		errno = EINVAL;
		if (endptr != NULL) {
			*endptr = (char *) nptr;
		}
		return 0;
	}
	
	/* Maximal value to which a digit can be added without a risk
	 * of overflow.
	 */
	uintmax_t max_safe_value = (real_max_value - base + 1) / base;
	
	uintmax_t result = 0;
	
	if (real_max_value == 0) {
		/* Special case when a negative number is parsed as
		 * unsigned integer. Only -0 is accepted.
		 */
		
		while (is_digit_in_base(nptr[i], base)) {
			if (nptr[i] != '0') {
				errno = ERANGE;
				result = 0;
			}
			i++;
		}
	}
	
	while (is_digit_in_base(nptr[i], base)) {
		int digit = digit_value(nptr[i]);
		
		if (result > max_safe_value) {
			/* corner case, check for overflow */
			
			uintmax_t boundary = (real_max_value - digit) / base;
			
			if (result > boundary) {
				/* overflow */
				errno = ERANGE;
				result = real_max_value;
				break;
			}
		}
		
		result = result * base + digit;
		i++;
	}
	
	if (endptr != NULL) {
		/* Move the pointer to the end of the number,
		 * in case it isn't there already.
		 */
		while (is_digit_in_base(nptr[i], base)) {
			i++;
		}
		
		*endptr = (char *) &nptr[i];
	}
	if (out_negative != NULL) {
		*out_negative = negative;
	}
	return result;
}

/**
 * Convert a string to an integer.
 *
 * @param nptr Input string.
 * @return Result of the conversion.
 */
int posix_atoi(const char *nptr)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, NULL, 10, INT_MIN, INT_MAX, &neg);

	return (neg ? ((int) -result) : (int) result);
}

/**
 * Convert a string to a long integer.
 *
 * @param nptr Input string.
 * @return Result of the conversion.
 */
long posix_atol(const char *nptr)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, NULL, 10, LONG_MIN, LONG_MAX, &neg);

	return (neg ? ((long) -result) : (long) result);
}

/**
 * Convert a string to a long long integer.
 *
 * @param nptr Input string.
 * @return Result of the conversion.
 */
long long posix_atoll(const char *nptr)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, NULL, 10, LLONG_MIN, LLONG_MAX, &neg);

	return (neg ? ((long long) -result) : (long long) result);
}

/**
 * Convert a string to a long integer.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
long posix_strtol(const char *restrict nptr, char **restrict endptr, int base)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, LONG_MIN, LONG_MAX, &neg);

	return (neg ? ((long) -result) : ((long) result));
}

/**
 * Convert a string to a long long integer.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
long long posix_strtoll(
    const char *restrict nptr, char **restrict endptr, int base)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, LLONG_MIN, LLONG_MAX, &neg);

	return (neg ? ((long long) -result) : (long long) result);
}

/**
 * Convert a string to a largest signed integer type.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
intmax_t posix_strtoimax(
    const char *restrict nptr, char **restrict endptr, int base)
{
	bool neg = false;
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, INTMAX_MIN, INTMAX_MAX, &neg);

	return (neg ? ((intmax_t) -result) : (intmax_t) result);
}

/**
 * Convert a string to an unsigned long integer.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
unsigned long posix_strtoul(
    const char *restrict nptr, char **restrict endptr, int base)
{
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, 0, ULONG_MAX, NULL);

	return (unsigned long) result;
}

/**
 * Convert a string to an unsigned long long integer.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
unsigned long long posix_strtoull(
    const char *restrict nptr, char **restrict endptr, int base)
{
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, 0, ULLONG_MAX, NULL);

	return (unsigned long long) result;
}

/**
 * Convert a string to a largest unsigned integer type.
 *
 * @param nptr Input string.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @param base Expected base of the string representation.
 * @return Result of the conversion.
 */
uintmax_t posix_strtoumax(
    const char *restrict nptr, char **restrict endptr, int base)
{
	uintmax_t result =
	    internal_strtol(nptr, endptr, base, 0, UINTMAX_MAX, NULL);

	return result;
}

/** @}
 */
