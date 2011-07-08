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
#include "../stdlib.h"

#include "../limits.h"
#include "../ctype.h"
#include "../errno.h"

// TODO: documentation

static inline bool is_digit_in_base(int c, int base)
{
	if (base <= 10) {
		return c >= '0' && c < '0' + base;
	} else {
		return isdigit(c) ||
		    (tolower(c) >= 'a' && tolower(c) < ('a' + base - 10));
	}
}

static inline int get_digit_in_base(int c, int base)
{
	if (c <= '9') {
		return c - '0';
	} else {
		return 10 + tolower(c) - 'a';
	}
}

static inline unsigned long long internal_strtol(
    const char *restrict nptr, char **restrict endptr, int base,
    const long long min_value, const unsigned long long max_value,
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
	
	unsigned long long real_max_value = max_value;
	
	/* Current index in the input string. */
	int i = 0;
	bool negative = false;
	
	/* Skip whitespace. */
	while (isspace(nptr[i])) {
		i++;
	}
	
	/* Parse sign. */
	switch (nptr[i]) {
	case '-':
		negative = true;
		real_max_value = -min_value;
		/* fallthrough */
	case '+':
		i++;
	}
	
	/* Figure out the base. */
	switch (base) {
	case 0:
		if (nptr[i] == '0') {
			if (tolower(nptr[i + 1]) == 'x') {
				base = 16;
				i += 2;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
		break;
	case 16:
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
	unsigned long long max_safe_value = (real_max_value - base + 1) / base;
	
	unsigned long long result = 0;
	
	if (real_max_value == 0) {
		/* Special case when a negative number is parsed as
		 * unsigned integer. Only -0 is accepted.
		 */
		
		while (is_digit_in_base(nptr[i], base)) {
			if (nptr[i] != '0') {
				errno = ERANGE;
				result = max_value;
			}
			i++;
		}
	}
	
	while (is_digit_in_base(nptr[i], base)) {
		int digit = get_digit_in_base(nptr[i], base);
		
		if (result > max_safe_value) {
			/* corner case, check for overflow */
			
			unsigned long long
			    boundary = (real_max_value - digit) / base;
			
			if (result > boundary) {
				/* overflow */
				errno = ERANGE;
				result = max_value;
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

int posix_atoi(const char *nptr)
{
	bool neg = false;
	unsigned long long result =
	    internal_strtol(nptr, NULL, 10, INT_MIN, INT_MAX, &neg);

	return (int) (neg ? -result : result);
}

long posix_atol(const char *nptr)
{
	bool neg = false;
	unsigned long long result =
	    internal_strtol(nptr, NULL, 10, LONG_MIN, LONG_MAX, &neg);

	return (long) (neg ? -result : result);
}

long long posix_atoll(const char *nptr)
{
	bool neg = false;
	unsigned long long result =
	    internal_strtol(nptr, NULL, 10, LLONG_MIN, LLONG_MAX, &neg);

	return (long long) (neg ? -result : result);
}

long posix_strtol(const char *restrict nptr, char **restrict endptr, int base)
{
	bool neg = false;
	unsigned long long result =
	    internal_strtol(nptr, endptr, base, LONG_MIN, LONG_MAX, &neg);

	return (long) (neg ? -result : result);
}

long long posix_strtoll(
    const char *restrict nptr, char **restrict endptr, int base)
{
	bool neg = false;
	unsigned long long result =
	    internal_strtol(nptr, endptr, base, LLONG_MIN, LLONG_MAX, &neg);

	return (long long) (neg ? -result : result);
}

unsigned long posix_strtoul(
    const char *restrict nptr, char **restrict endptr, int base)
{
	unsigned long long result =
	    internal_strtol(nptr, endptr, base, 0, ULONG_MAX, NULL);

	return (unsigned long) result;
}

unsigned long long posix_strtoull(
    const char *restrict nptr, char **restrict endptr, int base)
{
	return internal_strtol(nptr, endptr, base, 0, ULLONG_MAX, NULL);
}


/** @}
 */

