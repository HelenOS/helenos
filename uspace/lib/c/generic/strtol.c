/*
 * Copyright (c) 2005 Martin Decky
 * Copyright (c) 2008 Jiri Svoboda
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <inttypes.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

// TODO: unit tests

static inline int _digit_value(int c)
{
	if (isdigit(c)) {
		return c - '0';
	} else if (islower(c)) {
		return c - 'a' + 10;
	} else if (isupper(c)) {
		return c - 'A' + 10;
	}
	return INT_MAX;
}

/* FIXME: workaround for GCC "optimizing" the overflow check
 * into soft-emulated 128b multiplication using `__multi3`,
 * which we don't currently implement.
 */
__attribute__((noinline)) static uintmax_t _max_value(int base)
{
	return UINTMAX_MAX / base;
}

static inline uintmax_t _strtoumax(
    const char *restrict nptr, char **restrict endptr, int base,
    bool *restrict sgn)
{
	assert(nptr != NULL);
	assert(sgn != NULL);

	/* Skip leading whitespace. */

	while (isspace(*nptr)) {
		nptr++;
	}

	/* Parse sign, if any. */

	switch (*nptr) {
	case '-':
		*sgn = true;
		nptr++;
		break;
	case '+':
		nptr++;
		break;
	}

	/* Figure out the base. */

	if (base == 0) {
		if (*nptr == '0') {
			if (tolower(nptr[1]) == 'x') {
				/* 0x... is hex. */
				base = 16;
				nptr += 2;
			} else {
				/* 0... is octal. */
				base = 8;
			}
		} else {
			/* Anything else is decimal by default. */
			base = 10;
		}
	} else if (base == 16) {
		/* Allow hex number to be prefixed with "0x". */
		if (nptr[0] == '0' && tolower(nptr[1]) == 'x') {
			nptr += 2;
		}
	} else if (base < 0 || base == 1 || base > 36) {
		errno = EINVAL;
		return 0;
	}

	/* Read the value. */

	uintmax_t result = 0;
	uintmax_t max = _max_value(base);
	int digit;

	while (digit = _digit_value(*nptr), digit < base) {

		if (result > max ||
		    __builtin_add_overflow(result * base, digit, &result)) {

			errno = ERANGE;
			result = UINTMAX_MAX;
			break;
		}

		nptr++;
	}

	/* Set endptr. */

	if (endptr != NULL) {
		/* Move the pointer to the end of the number,
		 * in case it isn't there already.
		 */
		while (_digit_value(*nptr) < base) {
			nptr++;
		}

		*endptr = (char *) nptr;
	}

	return result;
}

static inline intmax_t _strtosigned(const char *nptr, char **endptr, int base,
    intmax_t min, intmax_t max)
{
	bool sgn = false;
	uintmax_t number = _strtoumax(nptr, endptr, base, &sgn);

	if (number > (uintmax_t) max) {
		if (sgn && (number - 1 == (uintmax_t) max)) {
			return min;
		}

		errno = ERANGE;
		return (sgn ? min : max);
	}

	return (sgn ? -number : number);
}

static inline uintmax_t _strtounsigned(const char *nptr, char **endptr, int base,
    uintmax_t max)
{
	bool sgn = false;
	uintmax_t number = _strtoumax(nptr, endptr, base, &sgn);

	if (sgn) {
		if (number == 0) {
			return 0;
		} else {
			errno = ERANGE;
			return max;
		}
	}

	if (number > max) {
		errno = ERANGE;
		return max;
	}

	return number;
}

/** Convert initial part of string to long int according to given base.
 * The number may begin with an arbitrary number of whitespaces followed by
 * optional sign (`+' or `-'). If the base is 0 or 16, the prefix `0x' may be
 * inserted and the number will be taken as hexadecimal one. If the base is 0
 * and the number begin with a zero, number will be taken as octal one (as with
 * base 8). Otherwise the base 0 is taken as decimal.
 *
 * @param nptr		Pointer to string.
 * @param[out] endptr	If not NULL, function stores here pointer to the first
 * 			invalid character.
 * @param base		Zero or number between 2 and 36 inclusive.
 * @return		Result of conversion.
 */
long strtol(const char *nptr, char **endptr, int base)
{
	return _strtosigned(nptr, endptr, base, LONG_MIN, LONG_MAX);
}

/** Convert initial part of string to unsigned long according to given base.
 * The number may begin with an arbitrary number of whitespaces followed by
 * optional sign (`+' or `-'). If the base is 0 or 16, the prefix `0x' may be
 * inserted and the number will be taken as hexadecimal one. If the base is 0
 * and the number begin with a zero, number will be taken as octal one (as with
 * base 8). Otherwise the base 0 is taken as decimal.
 *
 * @param nptr		Pointer to string.
 * @param[out] endptr	If not NULL, function stores here pointer to the first
 * 			invalid character
 * @param base		Zero or number between 2 and 36 inclusive.
 * @return		Result of conversion.
 */
unsigned long strtoul(const char *nptr, char **endptr, int base)
{
	return _strtounsigned(nptr, endptr, base, ULONG_MAX);
}

long long strtoll(const char *nptr, char **endptr, int base)
{
	return _strtosigned(nptr, endptr, base, LLONG_MIN, LLONG_MAX);
}

unsigned long long strtoull(const char *nptr, char **endptr, int base)
{
	return _strtounsigned(nptr, endptr, base, ULLONG_MAX);
}

intmax_t strtoimax(const char *nptr, char **endptr, int base)
{
	return _strtosigned(nptr, endptr, base, INTMAX_MIN, INTMAX_MAX);
}

uintmax_t strtoumax(const char *nptr, char **endptr, int base)
{
	return _strtounsigned(nptr, endptr, base, UINTMAX_MAX);
}

int atoi(const char *nptr)
{
	return (int)strtol(nptr, NULL, 10);
}

long atol(const char *nptr)
{
	return strtol(nptr, NULL, 10);
}

long long atoll(const char *nptr)
{
	return strtoll(nptr, NULL, 10);
}

/** @}
 */
