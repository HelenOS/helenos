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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <str.h>

// FIXME: The original HelenOS functions return EOVERFLOW instead
//        of ERANGE. It's a pointless distinction from standard functions,
//        so we should change that. Beware the callers though.

// TODO: more unit tests

static inline int isdigit(int c)
{
	return c >= '0' && c <= '9';
}

static inline int islower(int c)
{
	return c >= 'a' && c <= 'z';
}

static inline int isupper(int c)
{
	return c >= 'A' && c <= 'Z';
}

static inline int isspace(int c)
{
	switch (c) {
	case ' ':
	case '\n':
	case '\t':
	case '\f':
	case '\r':
	case '\v':
		return 1;
	default:
		return 0;
	}
}

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

/*
 * FIXME: workaround for GCC "optimizing" the overflow check
 * into soft-emulated 128b multiplication using `__multi3`,
 * which we don't currently implement.
 */
__attribute__((noinline)) static uintmax_t _max_value(int base)
{
	return UINTMAX_MAX / base;
}

static inline int _prefixbase(const char *restrict *nptrptr, bool nonstd)
{
	const char *nptr = *nptrptr;

	if (nptr[0] != '0')
		return 10;

	if (nptr[1] == 'x' || nptr[1] == 'X') {
		if (_digit_value(nptr[2]) < 16) {
			*nptrptr += 2;
			return 16;
		}
	}

	if (nonstd) {
		switch (nptr[1]) {
		case 'b':
		case 'B':
			if (_digit_value(nptr[2]) < 2) {
				*nptrptr += 2;
				return 2;
			}
			break;
		case 'o':
		case 'O':
			if (_digit_value(nptr[2]) < 8) {
				*nptrptr += 2;
				return 8;
			}
			break;
		case 'd':
		case 'D':
		case 't':
		case 'T':
			if (_digit_value(nptr[2]) < 10) {
				*nptrptr += 2;
				return 10;
			}
			break;
		}
	}

	return 8;
}

static inline uintmax_t _strtoumax(
    const char *restrict nptr, char **restrict endptr, int base,
    bool *restrict sgn, errno_t *err, bool nonstd)
{
	assert(nptr != NULL);
	assert(sgn != NULL);

	const char *first = nptr;

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

	if (base == 0)
		base = _prefixbase(&nptr, nonstd);

	if (base == 16 && !nonstd) {
		/*
		 * Standard strto* functions allow hexadecimal prefix to be
		 * present when base is explicitly set to 16.
		 * Our nonstandard str_* functions don't allow it.
		 * I don't know if that is intended, just matching the original
		 * functionality here.
		 */

		if (nptr[0] == '0' && (nptr[1] == 'x' || nptr[1] == 'X') &&
		    _digit_value(nptr[2]) < base)
			nptr += 2;
	}

	if (base < 2 || base > 36) {
		*err = EINVAL;
		return 0;
	}

	/* Must be at least one digit. */

	if (_digit_value(*nptr) >= base) {
		/* No digits on input. */
		if (endptr != NULL)
			*endptr = (char *) first;
		return 0;
	}

	/* Read the value.  */

	uintmax_t result = 0;
	uintmax_t max = _max_value(base);
	int digit;

	while (digit = _digit_value(*nptr), digit < base) {
		if (result > max ||
		    __builtin_add_overflow(result * base, digit, &result)) {

			*err = nonstd ? EOVERFLOW : ERANGE;
			result = UINTMAX_MAX;
			break;
		}

		nptr++;
	}

	/* Set endptr. */

	if (endptr != NULL) {
		/*
		 * Move the pointer to the end of the number,
		 * in case it isn't there already.
		 * This can happen when the number has legal formatting,
		 * but is out of range of the target type.
		 */
		while (_digit_value(*nptr) < base) {
			nptr++;
		}

		*endptr = (char *) nptr;
	}

	return result;
}

static inline intmax_t _strtosigned(const char *nptr, char **endptr, int base,
    intmax_t min, intmax_t max, errno_t *err, bool nonstd)
{
	bool sgn = false;
	uintmax_t number = _strtoumax(nptr, endptr, base, &sgn, err, nonstd);

	if (number > (uintmax_t) max) {
		if (sgn && (number - 1 == (uintmax_t) max)) {
			return min;
		}

		*err = nonstd ? EOVERFLOW : ERANGE;
		return (sgn ? min : max);
	}

	return (sgn ? -number : number);
}

static inline uintmax_t _strtounsigned(const char *nptr, char **endptr, int base,
    uintmax_t max, errno_t *err, bool nonstd)
{
	bool sgn = false;
	uintmax_t number = _strtoumax(nptr, endptr, base, &sgn, err, nonstd);

	if (nonstd && sgn) {
		/* Do not allow negative values */
		*err = EINVAL;
		return 0;
	}

	if (number > max) {
		*err = nonstd ? EOVERFLOW : ERANGE;
		return max;
	}

	return (sgn ? -number : number);
}

/** Convert string to uint64_t.
 *
 * @param nptr   Pointer to string.
 * @param endptr If not NULL, pointer to the first invalid character
 *               is stored here.
 * @param base   Zero or number between 2 and 36 inclusive.
 * @param strict Do not allow any trailing characters.
 * @param result Result of the conversion.
 *
 * @return EOK if conversion was successful.
 *
 */
errno_t str_uint64_t(const char *nptr, char **endptr, unsigned int base,
    bool strict, uint64_t *result)
{
	assert(result != NULL);

	errno_t rc = EOK;
	char *lendptr = (char *) nptr;

	uintmax_t r = _strtounsigned(nptr, &lendptr, base, UINT64_MAX, &rc, true);

	if (endptr)
		*endptr = lendptr;

	if (rc != EOK)
		return rc;

	if (strict && *lendptr != '\0')
		return EINVAL;

	*result = r;
	return EOK;
}

/** @}
 */
