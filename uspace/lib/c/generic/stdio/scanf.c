/*
 * Copyright (c) 2018 Jiri Svoboda
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
/**
 * @file
 * @brief Formatted input (scanf family)
 */

#include <assert.h>
#include <_bits/ssize_t.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../private/scanf.h"

typedef enum {
	/** No length modifier */
	lm_none,
	/** 'hh' */
	lm_hh,
	/** 'h' */
	lm_h,
	/** 'l' */
	lm_l,
	/** 'll' */
	lm_ll,
	/** 'j' */
	lm_j,
	/** 'z' */
	lm_z,
	/** 't' */
	lm_t,
	/** 'L' */
	lm_L
} lenmod_t;

typedef enum {
	/** Unknown */
	cs_unknown,
	/** 'd' */
	cs_decimal,
	/** 'i' */
	cs_int,
	/** 'o' */
	cs_octal,
	/** 'u' */
	cs_udecimal,
	/** 'x', 'X' */
	cs_hex,
	/** 'a', 'e', 'f', 'g', 'A', 'E', 'F', 'G' */
	cs_float,
	/** 'c' */
	cs_char,
	/** 's' */
	cs_str,
	/* [...] */
	cs_set,
	/* 'p' */
	cs_ptr,
	/* 'n' */
	cs_numchar,
	/* '%' */
	cs_percent
} cvtspcr_t;

/** Conversion specification */
typedef struct {
	/** Suppress assignment */
	bool noassign;
	/** Allocate memory for string (GNU extension) */
	bool memalloc;
	/** @c true if width field is valid */
	bool have_width;
	/** Width */
	size_t width;
	/** Length modifier */
	lenmod_t lenmod;
	/** Conversion specifier */
	cvtspcr_t spcr;
	/** Scan set (if spc == cs_set) */
	const char *scanset;
} cvtspec_t;

/** Buffer for writing strings */
typedef struct {
	/** Buffer */
	char *buf;
	/** Place where to store pointer to the buffer */
	char **pptr;
	/** Allocating memory for caller */
	bool memalloc;
	/** Current size of allocated buffer */
	size_t size;
} strbuf_t;

/** Wrapper needed to pass va_list around by reference in a portable fashion */
typedef struct {
	va_list ap;
} va_encaps_t;

static int digit_value(char digit)
{
	switch (digit) {
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	default:
		assert(false);
	}
}

static void cvtspec_parse(const char **fmt, cvtspec_t *spec)
{
	spec->scanset = NULL;
	spec->width = 0;

	/* Assignment suppresion */

	if (**fmt == '*') {
		spec->noassign = true;
		++(*fmt);
	} else {
		spec->noassign = false;
	}

	/* Memory allocation */

	if (**fmt == 'm') {
		spec->memalloc = true;
		++(*fmt);
	} else {
		spec->memalloc = false;
	}

	/* Width specifier */

	if (isdigit(**fmt)) {
		spec->have_width = true;
		assert(**fmt != '0');
		spec->width = 0;
		while (isdigit(**fmt)) {
			spec->width *= 10;
			spec->width += digit_value(**fmt);
			++(*fmt);
		}
	} else {
		spec->have_width = false;
	}

	/* Length modifier */

	switch (**fmt) {
	case 'h':
		++(*fmt);
		if (**fmt == 'h') {
			spec->lenmod = lm_hh;
			++(*fmt);
		} else {
			spec->lenmod = lm_h;
		}
		break;
	case 'l':
		++(*fmt);
		if (**fmt == 'l') {
			spec->lenmod = lm_ll;
			++(*fmt);
		} else {
			spec->lenmod = lm_l;
		}
		break;
	case 'j':
		++(*fmt);
		spec->lenmod = lm_j;
		break;
	case 'z':
		++(*fmt);
		spec->lenmod = lm_z;
		break;
	case 't':
		++(*fmt);
		spec->lenmod = lm_t;
		break;
	case 'L':
		++(*fmt);
		spec->lenmod = lm_L;
		break;
	default:
		spec->lenmod = lm_none;
		break;
	}

	/* Conversion specifier */

	switch (**fmt) {
	case 'd':
		++(*fmt);
		spec->spcr = cs_decimal;
		break;
	case 'i':
		++(*fmt);
		spec->spcr = cs_int;
		break;
	case 'o':
		++(*fmt);
		spec->spcr = cs_octal;
		break;
	case 'u':
		++(*fmt);
		spec->spcr = cs_udecimal;
		break;
	case 'x':
	case 'X':
		++(*fmt);
		spec->spcr = cs_hex;
		break;
	case 'a':
	case 'e':
	case 'f':
	case 'g':
	case 'A':
	case 'E':
	case 'F':
	case 'G':
		++(*fmt);
		spec->spcr = cs_float;
		break;
	case 'c':
		++(*fmt);
		spec->spcr = cs_char;
		break;
	case 's':
		++(*fmt);
		spec->spcr = cs_str;
		break;
	case '[':
		++(*fmt);
		spec->spcr = cs_set;
		spec->scanset = *fmt;
		while (**fmt != ']' && **fmt != '\0')
			++(*fmt);
		if (**fmt == ']')
			++(*fmt);
		break;
	case 'p':
		++(*fmt);
		spec->spcr = cs_ptr;
		break;
	case 'n':
		++(*fmt);
		spec->spcr = cs_numchar;
		break;
	case '%':
		++(*fmt);
		spec->spcr = cs_percent;
		break;
	default:
		assert(false);
		spec->spcr = cs_unknown;
		break;
	}
}

/** Initialize string buffer.
 *
 * String buffer is used to write characters from a string conversion.
 * The buffer can be provided by caller or dynamically allocated
 * (and grown).
 *
 * Initialize the string buffer @a strbuf. If @a spec->noassign,is true,
 * set the buffer pointer to NULL. Otherwise, if @a spec->memalloc is true,
 * allocate a buffer and read an argument of type char ** designating
 * a place where the pointer should be stored. If @a spec->memalloc is false,
 * read an argument of type char * and use it as a destination to write
 * the characters to.
 *
 * @param strbuf String buffer to initialize
 * @param cvtspec Conversion specification (noassign, memalloc)
 * @param ap Argument list to read pointer from
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t strbuf_init(strbuf_t *strbuf, cvtspec_t *spec, va_encaps_t *va)
{
	if (spec->noassign) {
		strbuf->memalloc = false;
		strbuf->buf = NULL;
		strbuf->pptr = NULL;
		strbuf->size = 0;
		return EOK;
	}

	if (spec->memalloc) {
		/* Allocate memory for caller */
		strbuf->memalloc = true;
		strbuf->size = 1;
		strbuf->buf = malloc(strbuf->size);
		if (strbuf->buf == NULL)
			return ENOMEM;

		/*
		 * Save pointer to allocated buffer to caller-provided
		 * location
		 */
		strbuf->pptr = va_arg(va->ap, char **);
		*strbuf->pptr = strbuf->buf;
	} else {
		/* Caller-provided buffer */
		strbuf->memalloc = false;
		strbuf->size = 0;
		strbuf->buf = va_arg(va->ap, char *);
		strbuf->pptr = NULL;
	}

	return EOK;
}

/** Write character at the specified position in a string buffer.
 *
 * The buffer is enlarged if necessary.
 *
 * @param strbuf String buffer
 * @param idx Character position (starting from 0)
 * @param c Character to write
 *
 * @return EOK on sucess, ENOMEM if out of memory
 */
static errno_t strbuf_write(strbuf_t *strbuf, size_t idx, char c)
{
	if (strbuf->memalloc && idx >= strbuf->size) {
		/* Enlarge buffer */
		strbuf->size = strbuf->size * 2;
		strbuf->buf = realloc(strbuf->buf, strbuf->size);
		*strbuf->pptr = strbuf->buf;
		if (strbuf->buf == NULL)
			return ENOMEM;
	}

	if (strbuf->buf != NULL)
		strbuf->buf[idx] = c;

	return EOK;
}

/** Get character from stream, keeping count of number of characters read.
 *
 * @param f Stream
 * @param numchar Pointer to counter of characters read
 * @return Character on success, EOF on error
 */
static int __fgetc(FILE *f, int *numchar)
{
	int c;

	c = fgetc(f);
	if (c == EOF)
		return EOF;

	++(*numchar);
	return c;
}

/** Unget character to stream, keeping count of number of characters read.
 *
 * @param c Character
 * @param f Stream
 * @param numchar Pointer to counter of characters read
 *
 * @return @a c on success, EOF on failure
 */
static int __ungetc(int c, FILE *f, int *numchar)
{
	int rc;

	rc = ungetc(c, f);
	if (rc == EOF)
		return EOF;

	--(*numchar);
	return rc;
}

/* Skip whitespace in input stream */
static errno_t vfscanf_skip_ws(FILE *f, int *numchar)
{
	int c;

	c = __fgetc(f, numchar);
	if (c == EOF)
		return EIO;

	while (isspace(c)) {
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;
	}

	if (c == EOF)
		return EIO;

	__ungetc(c, f, numchar);
	return EOK;
}

/* Match whitespace. */
static errno_t vfscanf_match_ws(FILE *f, int *numchar, const char **fmt)
{
	errno_t rc;

	rc = vfscanf_skip_ws(f, numchar);
	if (rc == EOF)
		return EIO;

	++(*fmt);
	return EOK;
}

/** Read intmax_t integer from file.
 *
 * @param f Input file
 * @param numchar Pointer to counter of characters read
 * @param base Numeric base (0 means detect using prefix)
 * @param width Maximum field with in characters
 * @param dest Place to store result
 * @return EOK on success, EIO on I/O error, EINVAL if input is not valid
 */
static errno_t __fstrtoimax(FILE *f, int *numchar, int base, size_t width,
    intmax_t *dest)
{
	errno_t rc;
	int c;
	intmax_t v;
	int digit;
	int sign;

	rc = vfscanf_skip_ws(f, numchar);
	if (rc == EIO)
		return EIO;

	c = __fgetc(f, numchar);
	if (c == EOF)
		return EIO;

	if (c == '+' || c == '-') {
		/* Sign */
		sign = (c == '-') ? -1 : +1;
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;
		--width;
	} else {
		sign = 1;
	}

	if (!isdigit(c) || width < 1) {
		__ungetc(c, f, numchar);
		return EINVAL;
	}

	if (base == 0) {
		/* Base prefix */
		if (c == '0') {
			c = __fgetc(f, numchar);
			if (c == EOF)
				return EIO;
			--width;

			if (width > 0 && (c == 'x' || c == 'X')) {
				--width;
				c = __fgetc(f, numchar);
				if (c == EOF)
					return EIO;
				if (width > 0 && isxdigit(c)) {
					base = 16;
				} else {
					*dest = 0;
					return EOK;
				}
			} else {
				base = 8;
				if (width == 0) {
					*dest = 0;
					return EOK;
				}
			}
		} else {
			base = 10;
		}
	}

	/* Value */
	v = 0;
	do {
		digit = digit_value(c);
		if (digit >= base)
			break;

		v = v * base + digit;
		c = __fgetc(f, numchar);
		--width;
	} while (width > 0 && isxdigit(c));

	if (c != EOF)
		__ungetc(c, f, numchar);

	*dest = sign * v;
	return EOK;
}

/** Read uintmax_t unsigned integer from file.
 *
 * @param f Input file
 * @param numchar Pointer to counter of characters read
 * @param base Numeric base (0 means detect using prefix)
 * @param width Maximum field with in characters
 * @param dest Place to store result
 * @return EOK on success, EIO on I/O error, EINVAL if input is not valid
 */
static errno_t __fstrtoumax(FILE *f, int *numchar, int base, size_t width,
    uintmax_t *dest)
{
	errno_t rc;
	int c;
	uintmax_t v;
	int digit;

	rc = vfscanf_skip_ws(f, numchar);
	if (rc == EIO)
		return EIO;

	c = __fgetc(f, numchar);
	if (c == EOF)
		return EIO;

	if (!isdigit(c) || width < 1) {
		__ungetc(c, f, numchar);
		return EINVAL;
	}

	if (base == 0) {
		/* Base prefix */
		if (c == '0') {
			c = __fgetc(f, numchar);
			if (c == EOF)
				return EIO;
			--width;

			if (width > 0 && (c == 'x' || c == 'X')) {
				--width;
				c = __fgetc(f, numchar);
				if (c == EOF)
					return EIO;
				if (width > 0 && isxdigit(c)) {
					base = 16;
				} else {
					*dest = 0;
					return EOK;
				}
			} else {
				base = 8;
				if (width == 0) {
					*dest = 0;
					return EOK;
				}
			}
		} else {
			base = 10;
		}
	}

	/* Value */
	v = 0;
	do {
		digit = digit_value(c);
		if (digit >= base)
			break;

		v = v * base + digit;
		c = __fgetc(f, numchar);
		--width;
	} while (width > 0 && isxdigit(c));

	if (c != EOF)
		__ungetc(c, f, numchar);

	*dest = v;
	return EOK;
}

/** Read long double from file.
 *
 * @param f Input file
 * @param numchar Pointer to counter of characters read
 * @param width Maximum field with in characters
 * @param dest Place to store result
 * @return EOK on success, EIO on I/O error, EINVAL if input is not valid
 */
errno_t __fstrtold(FILE *f, int *numchar, size_t width,
    long double *dest)
{
	errno_t rc;
	int c;
	long double v;
	int digit;
	int sign;
	int base;
	int efactor;
	int eadd;
	int eadj;
	int exp;
	int expsign;

	rc = vfscanf_skip_ws(f, numchar);
	if (rc == EIO)
		return EIO;

	c = __fgetc(f, numchar);
	if (c == EOF)
		return EIO;

	if (c == '+' || c == '-') {
		/* Sign */
		sign = (c == '-') ? -1 : +1;
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;
		--width;
	} else {
		sign = 1;
	}

	if (!isdigit(c) || width < 1) {
		__ungetc(c, f, numchar);
		return EINVAL;
	}

	/*
	 * Default is base 10
	 */

	/* Significand is in base 10 */
	base = 10;
	/* e+1 multiplies number by ten */
	efactor = 10;
	/* Adjust exp. by one for each fractional digit */
	eadd = 1;

	/* Base prefix */
	if (c == '0') {
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;
		--width;

		if (width > 0 && (c == 'x' || c == 'X')) {
			--width;
			c = __fgetc(f, numchar);

			if (width > 0 && isxdigit(c)) {
				/* Significand is in base 16 */
				base = 16;
				/* p+1 multiplies number by two */
				efactor = 2;
				/*
				 * Adjust exponent by 4 for each
				 * fractional digit
				 */
				eadd = 4;
			} else {
				*dest = 0;
				return EOK;
			}
		}
	}

	/* Value */
	v = 0;
	do {
		digit = digit_value(c);
		if (digit >= base)
			break;

		v = v * base + digit;
		c = __fgetc(f, numchar);
		--width;
	} while (width > 0 && isxdigit(c));

	/* Decimal-point */
	eadj = 0;

	if (c == '.' && width > 1) {
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;

		--width;

		/* Fractional part */
		while (width > 0 && isxdigit(c)) {
			digit = digit_value(c);
			if (digit >= base)
				break;

			v = v * base + digit;
			c = __fgetc(f, numchar);
			--width;
			eadj -= eadd;
		}
	}

	exp = 0;

	/* Exponent */
	if ((width > 1 && base == 10 && (c == 'e' || c == 'E')) ||
	    (width > 1 && base == 16 && (c == 'p' || c == 'P'))) {
		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;

		--width;

		if (width > 1 && (c == '+' || c == '-')) {
			/* Exponent sign */
			if (c == '+') {
				expsign = 1;
			} else {
				expsign = -1;
			}

			c = __fgetc(f, numchar);
			if (c == EOF)
				return EIO;

			--width;
		} else {
			expsign = 1;
		}

		while (width > 0 && isdigit(c)) {
			digit = digit_value(c);
			if (digit >= 10)
				break;

			exp = exp * 10 + digit;
			c = __fgetc(f, numchar);
			--width;
		}

		exp = exp * expsign;
	}

	exp += eadj;

	/* Adjust v for value of exponent */

	while (exp > 0) {
		v = v * efactor;
		--exp;
	}

	while (exp < 0) {
		v = v / efactor;
		++exp;
	}

	if (c != EOF)
		__ungetc(c, f, numchar);

	*dest = sign * v;
	return EOK;
}

/* Read characters from stream */
static errno_t __fgetchars(FILE *f, int *numchar, size_t width,
    strbuf_t *strbuf, size_t *nread)
{
	size_t cnt;
	int c;
	errno_t rc;

	*nread = 0;
	for (cnt = 0; cnt < width; cnt++) {
		c = __fgetc(f, numchar);
		if (c == EOF) {
			*nread = cnt;
			return EIO;
		}

		rc = strbuf_write(strbuf, cnt, c);
		if (rc != EOK) {
			*nread = cnt;
			return rc;
		}
	}

	*nread = cnt;
	return EOK;
}

/* Read non-whitespace string from stream */
static errno_t __fgetstr(FILE *f, int *numchar, size_t width, strbuf_t *strbuf,
    size_t *nread)
{
	size_t cnt;
	int c;
	errno_t rc;
	errno_t rc2;

	*nread = 0;

	rc = vfscanf_skip_ws(f, numchar);
	if (rc == EIO)
		return EIO;

	rc = EOK;

	for (cnt = 0; cnt < width; cnt++) {
		c = __fgetc(f, numchar);
		if (c == EOF) {
			rc = EIO;
			break;
		}

		if (isspace(c)) {
			__ungetc(c, f, numchar);
			break;
		}

		rc = strbuf_write(strbuf, cnt, c);
		if (rc != EOK) {
			*nread = cnt;
			return rc;
		}
	}

	/* Null-terminate */
	rc2 = strbuf_write(strbuf, cnt, '\0');
	if (rc2 != EOK) {
		*nread = cnt;
		return rc2;
	}

	*nread = cnt;
	return rc;
}

/** Determine if character is in scanset.
 *
 * Note that we support ranges, although that is a GNU extension.
 *
 * @param c Character
 * @param scanset Pointer to scanset
 * @return @c true iff @a c is in scanset @a scanset.
 */
static bool is_in_scanset(char c, const char *scanset)
{
	const char *p = scanset;
	bool inverted = false;
	char startc;
	char endc;

	/* Inverted scanset */
	if (*p == '^') {
		inverted = true;
		++p;
	}

	/*
	 * Either ']' or '-' at beginning or after '^' loses special meaning.
	 * However, '-' after ']' (or vice versa) does not.
	 */
	if (*p == ']') {
		/* ']' character in scanset */
		if (c == ']')
			return !inverted;
		++p;
	} else if (*p == '-') {
		/* '-' character in scanset */
		if (c == '-')
			return !inverted;
		++p;
	}

	/* Remaining characters */
	while (*p != '\0' && *p != ']') {
		/* '-' is a range unless it's the last character in scanset */
		if (*p == '-' && p[1] != ']' && p[1] != '\0') {
			startc = p[-1];
			endc = p[1];

			if (c >= startc && c <= endc)
				return !inverted;

			p += 2;
			continue;
		}

		if (*p == c)
			return !inverted;
		++p;
	}

	return inverted;
}

/* Read string of characters from scanset from stream */
static errno_t __fgetscanstr(FILE *f, int *numchar, size_t width,
    const char *scanset, strbuf_t *strbuf, size_t *nread)
{
	size_t cnt;
	int c;
	errno_t rc;
	errno_t rc2;

	rc = EOK;

	for (cnt = 0; cnt < width; cnt++) {
		c = __fgetc(f, numchar);
		if (c == EOF) {
			rc = EIO;
			break;
		}

		if (!is_in_scanset(c, scanset)) {
			__ungetc(c, f, numchar);
			break;
		}

		rc = strbuf_write(strbuf, cnt, c);
		if (rc != EOK) {
			*nread = cnt;
			break;
		}
	}

	/* Null-terminate */
	rc2 = strbuf_write(strbuf, cnt, '\0');
	if (rc2 != EOK) {
		*nread = cnt;
		return rc2;
	}

	*nread = cnt;
	return rc;
}

/** Perform a single conversion. */
static errno_t vfscanf_cvt(FILE *f, const char **fmt, va_encaps_t *va,
    int *numchar, unsigned *ncvt)
{
	errno_t rc;
	int c;
	intmax_t ival;
	uintmax_t uval;
	long double fval = 0.0;
	int *iptr;
	short *sptr;
	signed char *scptr;
	long *lptr;
	long long *llptr;
	ssize_t *ssptr;
	ptrdiff_t *pdptr;
	intmax_t *imptr;
	unsigned *uptr;
	unsigned short *usptr;
	unsigned char *ucptr;
	unsigned long *ulptr;
	unsigned long long *ullptr;
	float *fptr;
	double *dptr;
	long double *ldptr;
	size_t *szptr;
	ptrdiff_t *updptr;
	uintmax_t *umptr;
	void **pptr;
	size_t width;
	cvtspec_t cvtspec;
	strbuf_t strbuf;
	size_t nread;

	++(*fmt);

	cvtspec_parse(fmt, &cvtspec);

	width = cvtspec.have_width ? cvtspec.width : SIZE_MAX;

	switch (cvtspec.spcr) {
	case cs_percent:
		/* Match % character */
		rc = vfscanf_skip_ws(f, numchar);
		if (rc == EOF)
			return EIO;

		c = __fgetc(f, numchar);
		if (c == EOF)
			return EIO;
		if (c != '%') {
			__ungetc(c, f, numchar);
			return EINVAL;
		}
		break;
	case cs_decimal:
		/* Decimal integer */
		rc = __fstrtoimax(f, numchar, 10, width, &ival);
		if (rc != EOK)
			return rc;
		break;
	case cs_udecimal:
		/* Decimal unsigned integer */
		rc = __fstrtoumax(f, numchar, 10, width, &uval);
		if (rc != EOK)
			return rc;
		break;
	case cs_octal:
		/* Octal unsigned integer */
		rc = __fstrtoumax(f, numchar, 8, width, &uval);
		if (rc != EOK)
			return rc;
		break;
	case cs_hex:
		/* Hexadecimal unsigned integer */
		rc = __fstrtoumax(f, numchar, 16, width, &uval);
		if (rc != EOK)
			return rc;
		break;
	case cs_float:
		/* Floating-point value */
		rc = __fstrtold(f, numchar, width, &fval);
		if (rc != EOK)
			return rc;
		break;
	case cs_int:
		/* Signed integer with base detection */
		rc = __fstrtoimax(f, numchar, 0, width, &ival);
		if (rc != EOK)
			return rc;
		break;
	case cs_ptr:
		/* Unsigned integer with base detection (need 0xXXXXX) */
		rc = __fstrtoumax(f, numchar, 0, width, &uval);
		if (rc != EOK)
			return rc;
		break;
	case cs_char:
		/* Characters */
		rc = strbuf_init(&strbuf, &cvtspec, va);
		if (rc != EOK)
			return rc;

		width = cvtspec.have_width ? cvtspec.width : 1;
		rc = __fgetchars(f, numchar, width, &strbuf, &nread);
		if (rc != EOK) {
			if (rc == ENOMEM)
				return EIO;

			assert(rc == EIO);
			/* If we have at least one char, we succeeded */
			if (nread > 0)
				++(*ncvt);
			return EIO;
		}
		break;
	case cs_str:
		/* Non-whitespace string */
		rc = strbuf_init(&strbuf, &cvtspec, va);
		if (rc != EOK)
			return rc;

		width = cvtspec.have_width ? cvtspec.width : SIZE_MAX;
		rc = __fgetstr(f, numchar, width, &strbuf, &nread);
		if (rc != EOK) {
			if (rc == ENOMEM)
				return EIO;

			assert(rc == EIO);
			/* If we have at least one char, we succeeded */
			if (nread > 0)
				++(*ncvt);
			return EIO;
		}
		break;
	case cs_set:
		/* String of characters from scanset */
		rc = strbuf_init(&strbuf, &cvtspec, va);
		if (rc != EOK)
			return rc;

		width = cvtspec.have_width ? cvtspec.width : SIZE_MAX;
		rc = __fgetscanstr(f, numchar, width, cvtspec.scanset,
		    &strbuf, &nread);
		if (rc != EOK) {
			if (rc == ENOMEM)
				return EIO;

			assert(rc == EIO);
			/* If we have at least one char, we succeeded */
			if (nread > 0)
				++(*ncvt);
			return EIO;
		}
		break;
	case cs_numchar:
		break;
	default:
		return EINVAL;
	}

	/* Assignment */

	if (cvtspec.noassign)
		goto skip_assign;

	switch (cvtspec.spcr) {
	case cs_percent:
		break;
	case cs_decimal:
	case cs_int:
		switch (cvtspec.lenmod) {
		case lm_none:
			iptr = va_arg(va->ap, int *);
			*iptr = ival;
			break;
		case lm_hh:
			scptr = va_arg(va->ap, signed char *);
			*scptr = ival;
			break;
		case lm_h:
			sptr = va_arg(va->ap, short *);
			*sptr = ival;
			break;
		case lm_l:
			lptr = va_arg(va->ap, long *);
			*lptr = ival;
			break;
		case lm_ll:
			llptr = va_arg(va->ap, long long *);
			*llptr = ival;
			break;
		case lm_j:
			imptr = va_arg(va->ap, intmax_t *);
			*imptr = ival;
			break;
		case lm_z:
			ssptr = va_arg(va->ap, ssize_t *);
			*ssptr = ival;
			break;
		case lm_t:
			pdptr = va_arg(va->ap, ptrdiff_t *);
			*pdptr = ival;
			break;
		default:
			assert(false);
		}

		++(*ncvt);
		break;
	case cs_udecimal:
	case cs_octal:
	case cs_hex:
		switch (cvtspec.lenmod) {
		case lm_none:
			uptr = va_arg(va->ap, unsigned *);
			*uptr = uval;
			break;
		case lm_hh:
			ucptr = va_arg(va->ap, unsigned char *);
			*ucptr = uval;
			break;
		case lm_h:
			usptr = va_arg(va->ap, unsigned short *);
			*usptr = uval;
			break;
		case lm_l:
			ulptr = va_arg(va->ap, unsigned long *);
			*ulptr = uval;
			break;
		case lm_ll:
			ullptr = va_arg(va->ap, unsigned long long *);
			*ullptr = uval;
			break;
		case lm_j:
			umptr = va_arg(va->ap, uintmax_t *);
			*umptr = uval;
			break;
		case lm_z:
			szptr = va_arg(va->ap, size_t *);
			*szptr = uval;
			break;
		case lm_t:
			updptr = va_arg(va->ap, ptrdiff_t *);
			*updptr = uval;
			break;
		default:
			assert(false);
		}

		++(*ncvt);
		break;
	case cs_float:
		switch (cvtspec.lenmod) {
		case lm_none:
			fptr = va_arg(va->ap, float *);
			*fptr = fval;
			break;
		case lm_l:
			dptr = va_arg(va->ap, double *);
			*dptr = fval;
			break;
		case lm_L:
			ldptr = va_arg(va->ap, long double *);
			*ldptr = fval;
			break;
		default:
			assert(false);
		}

		++(*ncvt);
		break;
	case cs_ptr:
		pptr = va_arg(va->ap, void *);
		*pptr = (void *)(uintptr_t)uval;
		++(*ncvt);
		break;
	case cs_char:
		++(*ncvt);
		break;
	case cs_str:
		++(*ncvt);
		break;
	case cs_set:
		++(*ncvt);
		break;
	case cs_numchar:
		/* Store number of characters read so far. */
		iptr = va_arg(va->ap, int *);
		*iptr = *numchar;
		/* No incrementing of ncvt */
		break;
	default:
		return EINVAL;
	}

skip_assign:

	return EOK;
}

int vfscanf(FILE *f, const char *fmt, va_list ap)
{
	const char *cp;
	int c;
	unsigned ncvt;
	int numchar;
	bool input_error = false;
	va_encaps_t va;
	int rc;

	va_copy(va.ap, ap);

	ncvt = 0;
	numchar = 0;
	cp = fmt;
	while (*cp != '\0') {
		if (isspace(*cp)) {
			/* Whitespace */
			rc = vfscanf_match_ws(f, &numchar, &cp);
			if (rc == EIO) {
				input_error = true;
				break;
			}

			assert(rc == EOK);
		} else if (*cp == '%') {
			/* Conversion specification */
			rc = vfscanf_cvt(f, &cp, &va, &numchar,
			    &ncvt);
			if (rc == EIO) {
				/* Input error */
				input_error = true;
				break;
			}

			/* Other error */
			if (rc != EOK)
				break;
		} else {
			/* Match specific character */
			c = __fgetc(f, &numchar);
			if (c == EOF) {
				input_error = true;
				break;
			}

			if (c != *cp) {
				__ungetc(c, f, &numchar);
				break;
			}

			++cp;
		}
	}

	if (input_error && ncvt == 0)
		return EOF;

	return ncvt;
}

int fscanf(FILE *f, const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vfscanf(f, fmt, args);
	va_end(args);

	return rc;
}

int vscanf(const char *fmt, va_list ap)
{
	return vfscanf(stdin, fmt, ap);
}

int scanf(const char *fmt, ...)
{
	va_list args;
	int rc;

	va_start(args, fmt);
	rc = vscanf(fmt, args);
	va_end(args);

	return rc;
}

/** @}
 */
