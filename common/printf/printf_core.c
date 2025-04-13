/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2025 Jiří Zárevúcky
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
 * @brief Printing functions.
 */

#include <_bits/uchar.h>
#include <_bits/wint_t.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <macros.h>
#include <printf_core.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <str.h>

/* Disable float support in kernel, because we usually disable floating operations there. */
#if __STDC_HOSTED__
#define HAS_FLOAT
#endif

#ifdef HAS_FLOAT
#include <double_to_str.h>
#include <ieee_double.h>
#endif

/** show prefixes 0x or 0 */
#define __PRINTF_FLAG_PREFIX       0x00000001

/** show the decimal point even if no fractional digits present */
#define __PRINTF_FLAG_DECIMALPT    0x00000001

/** signed / unsigned number */
#define __PRINTF_FLAG_SIGNED       0x00000002

/** print leading zeroes */
#define __PRINTF_FLAG_ZEROPADDED   0x00000004

/** align to left */
#define __PRINTF_FLAG_LEFTALIGNED  0x00000010

/** always show + sign */
#define __PRINTF_FLAG_SHOWPLUS     0x00000020

/** print space instead of plus */
#define __PRINTF_FLAG_SPACESIGN    0x00000040

/** show big characters */
#define __PRINTF_FLAG_BIGCHARS     0x00000080

/** number has - sign */
#define __PRINTF_FLAG_NEGATIVE     0x00000100

/** don't print trailing zeros in the fractional part */
#define __PRINTF_FLAG_NOFRACZEROS  0x00000200

/**
 * Buffer big enough for 64-bit number printed in base 2.
 */
#define PRINT_NUMBER_BUFFER_SIZE  64

/** Get signed or unsigned integer argument */
#define PRINTF_GET_INT_ARGUMENT(type, ap, flags) \
	({ \
		unsigned type res; \
		\
		if ((flags) & __PRINTF_FLAG_SIGNED) { \
			signed type arg = va_arg((ap), signed type); \
			\
			if (arg < 0) { \
				res = -arg; \
				(flags) |= __PRINTF_FLAG_NEGATIVE; \
			} else \
				res = arg; \
		} else \
			res = va_arg((ap), unsigned type); \
		\
		res; \
	})

/** Enumeration of possible arguments types.
 */
typedef enum {
	PrintfQualifierByte = 0,
	PrintfQualifierShort,
	PrintfQualifierInt,
	PrintfQualifierLong,
	PrintfQualifierLongLong,
	PrintfQualifierPointer,
} qualifier_t;

static const char _digits_small[] = "0123456789abcdef";
static const char _digits_big[] = "0123456789ABCDEF";

static const char _nullstr[] = "(NULL)";
static const char _replacement[] = u8"�";
static const char _spaces[] = "                                               ";
static const char _zeros[] = "000000000000000000000000000000000000000000000000";

static void _set_errno(errno_t rc)
{
	#ifdef errno
	errno = rc;
	#endif
}

static size_t _utf8_bytes(char32_t c)
{
	if (c < 0x80)
		return 1;

	if (c < 0x800)
		return 2;

	if (c < 0xD800)
		return 3;

	/* Surrogate code points, invalid in UTF-32. */
	if (c < 0xE000)
		return sizeof(_replacement) - 1;

	if (c < 0x10000)
		return 3;

	if (c < 0x110000)
		return 4;

	/* Invalid character. */
	return sizeof(_replacement) - 1;
}

/** Counts characters and utf8 bytes in a wide string up to a byte limit.
 * @param max_bytes    Byte length limit for string's utf8 conversion.
 * @param[out] len     The number of wide characters
 * @return  Number of utf8 bytes that the first *len characters in the string
 *          will convert to. Will always be less than max_bytes.
 */
static size_t _utf8_wstr_bytes_len(char32_t *s, size_t max_bytes, size_t *len)
{
	size_t bytes = 0;
	size_t i;

	for (i = 0; bytes < max_bytes && s[i]; i++) {
		size_t next = _utf8_bytes(s[i]);
		if (max_bytes - bytes < next)
			break;

		bytes += next;
	}

	*len = i;
	return bytes;
}

#define TRY(expr) ({ errno_t rc = (expr); if (rc != EOK) return rc; })

static inline void _saturating_add(size_t *a, size_t b)
{
	size_t s = *a + b;
	/* Only works because size_t is unsigned. */
	*a = (s < b) ? SIZE_MAX : s;
}

static errno_t _write_bytes(const char *buf, size_t n, printf_spec_t *ps,
    size_t *written_bytes)
{
	errno_t rc = ps->write(buf, n, ps->data);
	if (rc != EOK)
		return rc;

	_saturating_add(written_bytes, n);
	return EOK;
}

/** Write one UTF-32 character. */
static errno_t _write_uchar(char32_t ch, printf_spec_t *ps,
    size_t *written_bytes)
{
	char utf8[4];
	size_t offset = 0;

	if (chr_encode(ch, utf8, &offset, sizeof(utf8)) == EOK)
		return _write_bytes(utf8, offset, ps, written_bytes);

	/* Invalid character. */
	return _write_bytes(_replacement, sizeof(_replacement) - 1, ps, written_bytes);
}

/** Write n UTF-32 characters. */
static errno_t _write_chars(const char32_t *buf, size_t n, printf_spec_t *ps,
    size_t *written_bytes)
{
	for (size_t i = 0; i < n; i++)
		TRY(_write_uchar(buf[i], ps, written_bytes));

	return EOK;
}

static errno_t _write_char(char c, printf_spec_t *ps, size_t *written_bytes)
{
	return _write_bytes(&c, 1, ps, written_bytes);
}

static errno_t _write_spaces(size_t n, printf_spec_t *ps, size_t *written_bytes)
{
	size_t max_spaces = sizeof(_spaces) - 1;

	while (n > max_spaces) {
		TRY(_write_bytes(_spaces, max_spaces, ps, written_bytes));
		n -= max_spaces;
	}

	return _write_bytes(_spaces, n, ps, written_bytes);
}

static errno_t _write_zeros(size_t n, printf_spec_t *ps, size_t *written_bytes)
{
	size_t max_zeros = sizeof(_zeros) - 1;

	while (n > max_zeros) {
		TRY(_write_bytes(_zeros, max_zeros, ps, written_bytes));
		n -= max_zeros;
	}

	return _write_bytes(_zeros, n, ps, written_bytes);
}

/** Print one formatted ASCII character.
 *
 * @param ch    Character to print.
 * @param width Width modifier.
 * @param flags Flags that change the way the character is printed.
 */
static errno_t _format_char(const char c, size_t width, uint32_t flags,
    printf_spec_t *ps, size_t *written_bytes)
{
	size_t bytes = 1;

	if (width <= bytes)
		return _write_char(c, ps, written_bytes);

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		TRY(_write_char(c, ps, written_bytes));
		TRY(_write_spaces(width - bytes, ps, written_bytes));
	} else {
		TRY(_write_spaces(width - bytes, ps, written_bytes));
		TRY(_write_char(c, ps, written_bytes));
	}

	return EOK;
}

/** Print one formatted wide character.
 *
 * @param ch    Character to print.
 * @param width Width modifier.
 * @param flags Flags that change the way the character is printed.
 */
static errno_t _format_uchar(const char32_t ch, size_t width, uint32_t flags,
    printf_spec_t *ps, size_t *written_bytes)
{
	/*
	 * All widths in printf() are specified in bytes. It might seem nonsensical
	 * with unicode text, but that's the way the function is defined. The width
     * is barely useful if you want column alignment in terminal, but keep in
     * mind that counting code points is only marginally better for that.
     * Characters can span more than one unicode code point, even in languages
     * based on latin alphabet, and a single unicode code point can occupy two
     * spaces in east asian scripts.
     *
     * What the width can actually be useful for is padding, when you need the
     * output to fill an exact number of bytes in a file. That use would break
     * if we did our own thing here.
     */

    size_t bytes = _utf8_bytes(ch);

	if (width <= bytes)
		return _write_uchar(ch, ps, written_bytes);

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		TRY(_write_uchar(ch, ps, written_bytes));
		TRY(_write_spaces(width - bytes, ps, written_bytes));
	} else {
		TRY(_write_spaces(width - bytes, ps, written_bytes));
		TRY(_write_uchar(ch, ps, written_bytes));
	}

	return EOK;
}

/** Print string.
 *
 * @param str       String to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 */
static errno_t _format_cstr(const char *str, size_t width, int precision,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	if (str == NULL)
		str = _nullstr;

	/* Negative precision == unspecified. */
	size_t max_bytes = (precision < 0) ? SIZE_MAX : (size_t) precision;
	size_t bytes = str_nsize(str, max_bytes);

	if (width <= bytes)
		return _write_bytes(str, bytes, ps, written_bytes);

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		TRY(_write_bytes(str, bytes, ps, written_bytes));
		TRY(_write_spaces(width - bytes, ps, written_bytes));
	} else {
		TRY(_write_spaces(width - bytes, ps, written_bytes));
		TRY(_write_bytes(str, bytes, ps, written_bytes));
	}

	return EOK;
}

/** Print wide string.
 *
 * @param str       Wide string to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 */
static errno_t _format_wstr(char32_t *str, size_t width, int precision,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	if (!str)
		return _format_cstr(_nullstr, width, precision, flags, ps, written_bytes);

	/* Width and precision are always byte-based. See _format_uchar() */
	/* Negative precision == unspecified. */
	size_t max_bytes = (precision < 0) ? SIZE_MAX : (size_t) precision;

	size_t len;
	size_t bytes = _utf8_wstr_bytes_len(str, max_bytes, &len);

	if (width <= bytes)
		return _write_chars(str, len, ps, written_bytes);

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		TRY(_write_chars(str, len, ps, written_bytes));
		TRY(_write_spaces(width - bytes, ps, written_bytes));
	} else {
		TRY(_write_spaces(width - bytes, ps, written_bytes));
		TRY(_write_chars(str, len, ps, written_bytes));
	}

	return EOK;
}

static char _sign(uint32_t flags)
{
	if (!(flags & __PRINTF_FLAG_SIGNED))
		return 0;

	if (flags & __PRINTF_FLAG_NEGATIVE)
		return '-';

	if (flags & __PRINTF_FLAG_SHOWPLUS)
		return '+';

	if (flags & __PRINTF_FLAG_SPACESIGN)
		return ' ';

	return 0;
}

/** Print a number in a given base.
 *
 * Print significant digits of a number in given base.
 *
 * @param num       Number to print.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param base      Base to print the number in (must be between 2 and 16).
 * @param flags     Flags that modify the way the number is printed.
 */
static errno_t _format_number(uint64_t num, size_t width, int precision, int base,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	assert(base >= 2 && base <= 16);

	/* Default precision for numeric output is 1. */
	size_t min_digits = (precision < 0) ? 1 : precision;

	bool bigchars = flags & __PRINTF_FLAG_BIGCHARS;
	bool prefix = flags & __PRINTF_FLAG_PREFIX;
	bool left_aligned = flags & __PRINTF_FLAG_LEFTALIGNED;
	bool zero_padded = flags & __PRINTF_FLAG_ZEROPADDED;

	const char *digits = bigchars ? _digits_big : _digits_small;

	char buffer[PRINT_NUMBER_BUFFER_SIZE];
	char *end = &buffer[PRINT_NUMBER_BUFFER_SIZE];

	/* Write number to the buffer. */
	int offset = 0;
	while (num > 0) {
		end[--offset] = digits[num % base];
		num /= base;
	}

	char *number = &end[offset];
	size_t number_len = end - number;
	char sign = _sign(flags);

	if (left_aligned) {
		/* Space padded right-aligned. */
		size_t real_size = max(number_len, min_digits);

		if (sign) {
			TRY(_write_char(sign, ps, written_bytes));
			real_size++;
		}

		if (prefix && base == 2 && number_len > 0) {
			TRY(_write_bytes(bigchars ? "0B" : "0b", 2, ps, written_bytes));
			real_size += 2;
		}

		if (prefix && base == 16 && number_len > 0) {
			TRY(_write_bytes(bigchars ? "0X" : "0x", 2, ps, written_bytes));
			real_size += 2;
		}

		if (min_digits > number_len) {
			TRY(_write_zeros(min_digits - number_len, ps, written_bytes));
		} else if (prefix && base == 8) {
			TRY(_write_zeros(1, ps, written_bytes));
			real_size++;
		}

		TRY(_write_bytes(number, number_len, ps, written_bytes));

		if (width > real_size)
			TRY(_write_spaces(width - real_size, ps, written_bytes));

		return EOK;
	}

	/* Zero padded number (ignored when left aligned or if precision is specified). */
	if (precision < 0 && zero_padded) {
		size_t real_size = number_len;

		if (sign) {
			TRY(_write_char(sign, ps, written_bytes));
			real_size++;
		}

		if (prefix && base == 2 && number_len > 0) {
			TRY(_write_bytes(bigchars ? "0B" : "0b", 2, ps, written_bytes));
			real_size += 2;
		}

		if (prefix && base == 16 && number_len > 0) {
			TRY(_write_bytes(bigchars ? "0X" : "0x", 2, ps, written_bytes));
			real_size += 2;
		}

		if (width > real_size)
			TRY(_write_zeros(width - real_size, ps, written_bytes));
		else if (number_len == 0 || (prefix && base == 8))
			TRY(_write_char('0', ps, written_bytes));

		return _write_bytes(number, number_len, ps, written_bytes);
	}

	/* Space padded right-aligned. */
	size_t real_size = max(number_len, min_digits);
	if (sign)
		real_size++;

	if (prefix && (base == 2 || base == 16) && number_len > 0)
		real_size += 2;

	if (prefix && base == 8 && number_len >= min_digits)
		real_size += 1;

	if (width > real_size)
		TRY(_write_spaces(width - real_size, ps, written_bytes));

	if (sign)
		TRY(_write_char(sign, ps, written_bytes));

	if (prefix && base == 2 && number_len > 0)
		TRY(_write_bytes(bigchars ? "0B" : "0b", 2, ps, written_bytes));

	if (prefix && base == 16 && number_len > 0)
		TRY(_write_bytes(bigchars ? "0X" : "0x", 2, ps, written_bytes));

	if (min_digits > number_len)
		TRY(_write_zeros(min_digits - number_len, ps, written_bytes));
	else if (prefix && base == 8)
		TRY(_write_char('0', ps, written_bytes));

	return _write_bytes(number, number_len, ps, written_bytes);
}

#ifdef HAS_FLOAT

/** Unformatted double number string representation. */
typedef struct {
	/** Buffer with len digits, no sign or leading zeros. */
	char *str;
	/** Number of digits in str. */
	int len;
	/** Decimal exponent, ie number = str * 10^dec_exp */
	int dec_exp;
	/** True if negative. */
	bool neg;
} double_str_t;

/** Returns the sign character or 0 if no sign should be printed. */
static char _get_sign_char(bool negative, uint32_t flags)
{
	if (negative) {
		return '-';
	} else if (flags & __PRINTF_FLAG_SHOWPLUS) {
		return '+';
	} else if (flags & __PRINTF_FLAG_SPACESIGN) {
		return ' ';
	} else {
		return 0;
	}
}

/** Prints a special double (ie NaN, infinity) padded to width characters. */
static errno_t _format_special(ieee_double_t val, int width, uint32_t flags,
    printf_spec_t *ps, size_t *written_bytes)
{
	assert(val.is_special);

	char sign = _get_sign_char(val.is_negative, flags);

	const int str_len = 3;
	const char *str;

	if (flags & __PRINTF_FLAG_BIGCHARS) {
		str = val.is_infinity ? "INF" : "NAN";
	} else {
		str = val.is_infinity ? "inf" : "nan";
	}

	int padding_len = max(0, width - ((sign ? 1 : 0) + str_len));

	/* Leading padding. */
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED))
		TRY(_write_spaces(padding_len, ps, written_bytes));

	if (sign)
		TRY(_write_char(sign, ps, written_bytes));

	TRY(_write_bytes(str, str_len, ps, written_bytes));

	/* Trailing padding. */
	if (flags & __PRINTF_FLAG_LEFTALIGNED)
		TRY(_write_spaces(padding_len, ps, written_bytes));

	return EOK;
}

/** Trims trailing zeros but leaves a single "0" intact. */
static void _fp_trim_trailing_zeros(char *buf, int *len, int *dec_exp)
{
	/* Cut the zero off by adjusting the exponent. */
	while (2 <= *len && '0' == buf[*len - 1]) {
		--*len;
		++*dec_exp;
	}
}

/** Textually round up the last digit thereby eliminating it. */
static void _fp_round_up(char *buf, int *len, int *dec_exp)
{
	assert(1 <= *len);

	char *last_digit = &buf[*len - 1];

	int carry = ('5' <= *last_digit);

	/* Cut the digit off by adjusting the exponent. */
	--*len;
	++*dec_exp;
	--last_digit;

	if (carry) {
		/* Skip all the digits to cut off/round to zero. */
		while (buf <= last_digit && '9' == *last_digit) {
			--last_digit;
		}

		/* last_digit points to the last digit to round but not '9' */
		if (buf <= last_digit) {
			*last_digit += 1;
			int new_len = last_digit - buf + 1;
			*dec_exp += *len - new_len;
			*len = new_len;
		} else {
			/* All len digits rounded to 0. */
			buf[0] = '1';
			*dec_exp += *len;
			*len = 1;
		}
	} else {
		/* The only digit was rounded to 0. */
		if (last_digit < buf) {
			buf[0] = '0';
			*dec_exp = 0;
			*len = 1;
		}
	}
}

/** Format and print the double string repressentation according
 *  to the %f specifier.
 */
static errno_t _format_double_str_fixed(double_str_t *val_str, int precision, int width,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	int len = val_str->len;
	char *buf = val_str->str;
	int dec_exp = val_str->dec_exp;

	assert(0 < len);
	assert(0 <= precision);
	assert(0 <= dec_exp || -dec_exp <= precision);

	/* Number of integral digits to print (at least leading zero). */
	int int_len = max(1, len + dec_exp);

	char sign = _get_sign_char(val_str->neg, flags);

	/* Fractional portion lengths. */
	int last_frac_signif_pos = max(0, -dec_exp);
	int leading_frac_zeros = max(0, last_frac_signif_pos - len);
	int signif_frac_figs = min(last_frac_signif_pos, len);
	int trailing_frac_zeros = precision - last_frac_signif_pos;
	char *buf_frac = buf + len - signif_frac_figs;

	if (flags & __PRINTF_FLAG_NOFRACZEROS)
		trailing_frac_zeros = 0;

	int frac_len = leading_frac_zeros + signif_frac_figs + trailing_frac_zeros;

	bool has_decimal_pt = (0 < frac_len) || (flags & __PRINTF_FLAG_DECIMALPT);

	/* Number of non-padding chars to print. */
	int num_len = (sign ? 1 : 0) + int_len + (has_decimal_pt ? 1 : 0) + frac_len;

	int padding_len = max(0, width - num_len);

	/* Leading padding and sign. */

	if (!(flags & (__PRINTF_FLAG_LEFTALIGNED | __PRINTF_FLAG_ZEROPADDED)))
		TRY(_write_spaces(padding_len, ps, written_bytes));

	if (sign)
		TRY(_write_char(sign, ps, written_bytes));

	if (flags & __PRINTF_FLAG_ZEROPADDED)
		TRY(_write_zeros(padding_len, ps, written_bytes));

	/* Print the intergral part of the buffer. */

	int buf_int_len = min(len, len + dec_exp);

	if (0 < buf_int_len) {
		TRY(_write_bytes(buf, buf_int_len, ps, written_bytes));

		/* Print trailing zeros of the integral part of the number. */
		TRY(_write_zeros(int_len - buf_int_len, ps, written_bytes));
	} else {
		/* Single leading integer 0. */
		TRY(_write_char('0', ps, written_bytes));
	}

	/* Print the decimal point and the fractional part. */
	if (has_decimal_pt) {
		TRY(_write_char('.', ps, written_bytes));

		/* Print leading zeros of the fractional part of the number. */
		TRY(_write_zeros(leading_frac_zeros, ps, written_bytes));

		/* Print significant digits of the fractional part of the number. */
		if (0 < signif_frac_figs)
			TRY(_write_bytes(buf_frac, signif_frac_figs, ps, written_bytes));

		/* Print trailing zeros of the fractional part of the number. */
		TRY(_write_zeros(trailing_frac_zeros, ps, written_bytes));
	}

	/* Trailing padding. */
	if (flags & __PRINTF_FLAG_LEFTALIGNED)
		TRY(_write_spaces(padding_len, ps, written_bytes));

	return EOK;
}

/** Convert, format and print a double according to the %f specifier.
 *
 * @param g     Double to print.
 * @param precision Number of fractional digits to print. If 0 no
 *              decimal point will be printed unless the flag
 *              __PRINTF_FLAG_DECIMALPT is specified.
 * @param width Minimum number of characters to display. Pads
 *              with '0' or ' ' depending on the set flags;
 * @param flags Printf flags.
 * @param ps    Printing functions.
 */
static errno_t _format_double_fixed(double g, int precision, int width,
	uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		flags &= ~__PRINTF_FLAG_ZEROPADDED;
	}

	if (flags & __PRINTF_FLAG_DECIMALPT) {
		flags &= ~__PRINTF_FLAG_NOFRACZEROS;
	}

	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special) {
		return _format_special(val, width, flags, ps, written_bytes);
	}

	char buf[MAX_DOUBLE_STR_BUF_SIZE];
	const size_t buf_size = MAX_DOUBLE_STR_BUF_SIZE;
	double_str_t val_str;

	val_str.str = buf;
	val_str.neg = val.is_negative;

	if (0 <= precision) {
		/*
		 * Request one more digit so we can round the result. The last
		 * digit it returns may have an error of at most +/- 1.
		 */
		val_str.len = double_to_fixed_str(val, -1, precision + 1, buf, buf_size,
		    &val_str.dec_exp);

		/*
		 * Round using the last digit to produce precision fractional digits.
		 * If less than precision+1 fractional digits were output the last
		 * digit is definitely inaccurate so also round to get rid of it.
		 */
		_fp_round_up(buf, &val_str.len, &val_str.dec_exp);

		/* Rounding could have introduced trailing zeros. */
		if (flags & __PRINTF_FLAG_NOFRACZEROS) {
			_fp_trim_trailing_zeros(buf, &val_str.len, &val_str.dec_exp);
		}
	} else {
		/* Let the implementation figure out the proper precision. */
		val_str.len = double_to_short_str(val, buf, buf_size, &val_str.dec_exp);

		/* Precision needed for the last significant digit. */
		precision = max(0, -val_str.dec_exp);
	}

	return _format_double_str_fixed(&val_str, precision, width, flags, ps, written_bytes);
}

/** Prints the decimal exponent part of a %e specifier formatted number. */
static errno_t _format_exponent(int exp_val, uint32_t flags, printf_spec_t *ps,
    size_t *written_bytes)
{
	char exp_ch = (flags & __PRINTF_FLAG_BIGCHARS) ? 'E' : 'e';
	TRY(_write_char(exp_ch, ps, written_bytes));

	char exp_sign = (exp_val < 0) ? '-' : '+';
	TRY(_write_char(exp_sign, ps, written_bytes));

	/* Print the exponent. */
	exp_val = abs(exp_val);

	char exp_str[4] = { 0 };

	exp_str[0] = '0' + exp_val / 100;
	exp_str[1] = '0' + (exp_val % 100) / 10;
	exp_str[2] = '0' + (exp_val % 10);

	int exp_len = (exp_str[0] == '0') ? 2 : 3;
	const char *exp_str_start = &exp_str[3] - exp_len;

	return _write_bytes(exp_str_start, exp_len, ps, written_bytes);
}

/** Format and print the double string repressentation according
 *  to the %e specifier.
 */
static errno_t _format_double_str_scient(double_str_t *val_str, int precision,
    int width, uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	int len = val_str->len;
	int dec_exp = val_str->dec_exp;
	char *buf  = val_str->str;

	assert(0 < len);

	char sign = _get_sign_char(val_str->neg, flags);
	bool has_decimal_pt = (0 < precision) || (flags & __PRINTF_FLAG_DECIMALPT);
	int dec_pt_len = has_decimal_pt ? 1 : 0;

	/* Fractional part lengths. */
	int signif_frac_figs = len - 1;
	int trailing_frac_zeros = precision - signif_frac_figs;

	if (flags & __PRINTF_FLAG_NOFRACZEROS) {
		trailing_frac_zeros = 0;
	}

	int frac_len = signif_frac_figs + trailing_frac_zeros;

	int exp_val = dec_exp + len - 1;
	/* Account for exponent sign and 'e'; minimum 2 digits. */
	int exp_len = 2 + (abs(exp_val) >= 100 ? 3 : 2);

	/* Number of non-padding chars to print. */
	int num_len = (sign ? 1 : 0) + 1 + dec_pt_len + frac_len + exp_len;

	int padding_len = max(0, width - num_len);

	if (!(flags & (__PRINTF_FLAG_LEFTALIGNED | __PRINTF_FLAG_ZEROPADDED)))
		TRY(_write_spaces(padding_len, ps, written_bytes));

	if (sign)
		TRY(_write_char(sign, ps, written_bytes));

	if (flags & __PRINTF_FLAG_ZEROPADDED)
		TRY(_write_zeros(padding_len, ps, written_bytes));

	/* Single leading integer. */
	TRY(_write_char(buf[0], ps, written_bytes));

	/* Print the decimal point and the fractional part. */
	if (has_decimal_pt) {
		TRY(_write_char('.', ps, written_bytes));

		/* Print significant digits of the fractional part of the number. */
		if (0 < signif_frac_figs)
			TRY(_write_bytes(buf + 1, signif_frac_figs, ps, written_bytes));

		/* Print trailing zeros of the fractional part of the number. */
		TRY(_write_zeros(trailing_frac_zeros, ps, written_bytes));
	}

	/* Print the exponent. */
	TRY(_format_exponent(exp_val, flags, ps, written_bytes));

	if (flags & __PRINTF_FLAG_LEFTALIGNED)
		TRY(_write_spaces(padding_len, ps, written_bytes));

	return EOK;
}

/** Convert, format and print a double according to the %e specifier.
 *
 * Note that if g is large, the output may be huge (3e100 prints
 * with at least 100 digits).
 *
 * %e style: [-]d.dddde+dd
 *  left-justified:  [-]d.dddde+dd[space_pad]
 *  right-justified: [space_pad][-][zero_pad]d.dddde+dd
 *
 * @param g     Double to print.
 * @param precision Number of fractional digits to print, ie
 *              precision + 1 significant digits to display. If 0 no
 *              decimal point will be printed unless the flag
 *              __PRINTF_FLAG_DECIMALPT is specified. If negative
 *              the shortest accurate number will be printed.
 * @param width Minimum number of characters to display. Pads
 *              with '0' or ' ' depending on the set flags;
 * @param flags Printf flags.
 * @param ps    Printing functions.
 */
static errno_t _format_double_scientific(double g, int precision, int width,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	if (flags & __PRINTF_FLAG_LEFTALIGNED)
		flags &= ~__PRINTF_FLAG_ZEROPADDED;

	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special)
		return _format_special(val, width, flags, ps, written_bytes);

	char buf[MAX_DOUBLE_STR_BUF_SIZE];
	const size_t buf_size = MAX_DOUBLE_STR_BUF_SIZE;
	double_str_t val_str;

	val_str.str = buf;
	val_str.neg = val.is_negative;

	if (0 <= precision) {
		/*
		 * Request one more digit (in addition to the leading integer)
		 * so we can round the result. The last digit it returns may
		 * have an error of at most +/- 1.
		 */
		val_str.len = double_to_fixed_str(val, precision + 2, -1, buf, buf_size,
		    &val_str.dec_exp);

		/*
		 * Round the extra digit to produce precision+1 significant digits.
		 * If less than precision+2 significant digits were returned the last
		 * digit is definitely inaccurate so also round to get rid of it.
		 */
		_fp_round_up(buf, &val_str.len, &val_str.dec_exp);

		/* Rounding could have introduced trailing zeros. */
		if (flags & __PRINTF_FLAG_NOFRACZEROS) {
			_fp_trim_trailing_zeros(buf, &val_str.len, &val_str.dec_exp);
		}
	} else {
		/* Let the implementation figure out the proper precision. */
		val_str.len = double_to_short_str(val, buf, buf_size, &val_str.dec_exp);

		/* Use all produced digits. */
		precision = val_str.len - 1;
	}

	return _format_double_str_scient(&val_str, precision, width, flags, ps, written_bytes);
}

/** Convert, format and print a double according to the %g specifier.
 *
 * %g style chooses between %f and %e.
 *
 * @param g     Double to print.
 * @param precision Number of significant digits to display; excluding
 *              any leading zeros from this count. If negative
 *              the shortest accurate number will be printed.
 * @param width Minimum number of characters to display. Pads
 *              with '0' or ' ' depending on the set flags;
 * @param flags Printf flags.
 * @param ps    Printing functions.
 *
 * @return The number of characters printed; negative on failure.
 */
static errno_t _format_double_generic(double g, int precision, int width,
    uint32_t flags, printf_spec_t *ps, size_t *written_bytes)
{
	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special)
		return _format_special(val, width, flags, ps, written_bytes);

	char buf[MAX_DOUBLE_STR_BUF_SIZE];
	const size_t buf_size = MAX_DOUBLE_STR_BUF_SIZE;
	int dec_exp;
	int len;

	/* Honor the user requested number of significant digits. */
	if (0 <= precision) {
		/*
		 * Do a quick and dirty conversion of a single digit to determine
		 * the decimal exponent.
		 */
		len = double_to_fixed_str(val, 1, -1, buf, buf_size, &dec_exp);
		assert(0 < len);

		precision = max(1, precision);

		if (-4 <= dec_exp && dec_exp < precision) {
			precision = precision - (dec_exp + 1);
			return _format_double_fixed(g, precision, width,
			    flags | __PRINTF_FLAG_NOFRACZEROS, ps, written_bytes);
		} else {
			--precision;
			return _format_double_scientific(g, precision, width,
			    flags | __PRINTF_FLAG_NOFRACZEROS, ps, written_bytes);
		}
	} else {
		/* Convert to get the decimal exponent and digit count. */
		len = double_to_short_str(val, buf, buf_size, &dec_exp);
		assert(0 < len);

		if (flags & __PRINTF_FLAG_LEFTALIGNED) {
			flags &= ~__PRINTF_FLAG_ZEROPADDED;
		}

		double_str_t val_str;
		val_str.str = buf;
		val_str.len = len;
		val_str.neg = val.is_negative;
		val_str.dec_exp = dec_exp;

		int first_digit_pos = len + dec_exp;
		int last_digit_pos = dec_exp;

		/* The whole number (15 digits max) fits between dec places 15 .. -6 */
		if (len <= 15 && -6 <= last_digit_pos && first_digit_pos <= 15) {
			/* Precision needed for the last significant digit. */
			precision = max(0, -val_str.dec_exp);
			return _format_double_str_fixed(&val_str, precision, width, flags, ps, written_bytes);
		} else {
			/* Use all produced digits. */
			precision = val_str.len - 1;
			return _format_double_str_scient(&val_str, precision, width, flags, ps, written_bytes);
		}
	}
}

/** Convert, format and print a double according to the specifier.
 *
 * Depending on the specifier it prints the double using the styles
 * %g, %f or %e by means of print_double_generic(), print_double_fixed(),
 * print_double_scientific().
 *
 * @param g     Double to print.
 * @param spec  Specifier of the style to print in; one of: 'g','G','f','F',
 *              'e','E'.
 * @param precision Number of fractional digits to display. If negative
 *              the shortest accurate number will be printed for style %g;
 *              negative precision defaults to 6 for styles %f, %e.
 * @param width Minimum number of characters to display. Pads
 *              with '0' or ' ' depending on the set flags;
 * @param flags Printf flags.
 * @param ps    Printing functions.
 */
static errno_t _format_double(double g, char spec, int precision, int width,
    uint32_t flags, printf_spec_t *ps, size_t *written_chars)
{
	switch (spec) {
	case 'F':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'f':
		precision = (precision < 0) ? 6 : precision;
		return _format_double_fixed(g, precision, width, flags, ps, written_chars);

	case 'E':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'e':
		precision = (precision < 0) ? 6 : precision;
		return _format_double_scientific(g, precision, width, flags, ps, written_chars);

	case 'G':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'g':
		return _format_double_generic(g, precision, width, flags, ps, written_chars);

	default:
		assert(false);
		return -1;
	}
}

#endif

static const char *_strchrnul(const char *s, int c)
{
	while (*s != c && *s != 0)
		s++;
	return s;
}

/** Read a sequence of digits from the format string as a number.
 * If the number has too many digits to fit in int, returns INT_MAX.
 */
static int _read_num(const char *fmt, size_t *i)
{
	const char *s;
	unsigned n = 0;

	for (s = &fmt[*i]; isdigit(*s); s++) {
		unsigned digit = (*s - '0');

		/* Check for overflow */
		if (n > INT_MAX / 10 || n * 10 > INT_MAX - digit) {
			n = INT_MAX;
			while (isdigit(*s))
				s++;
			break;
		}

		n = n * 10 + digit;
	}

	*i = s - fmt;
	return n;
}

static uint32_t _parse_flags(const char *fmt, size_t *i)
{
	uint32_t flags = 0;

	while (true) {
		switch (fmt[(*i)++]) {
		case '#':
			flags |= __PRINTF_FLAG_PREFIX;
			flags |= __PRINTF_FLAG_DECIMALPT;
			continue;
		case '-':
			flags |= __PRINTF_FLAG_LEFTALIGNED;
			continue;
		case '+':
			flags |= __PRINTF_FLAG_SHOWPLUS;
			continue;
		case ' ':
			flags |= __PRINTF_FLAG_SPACESIGN;
			continue;
		case '0':
			flags |= __PRINTF_FLAG_ZEROPADDED;
			continue;
		}

		--*i;
		break;
	}

	return flags;
}

static bool _eat_char(const char *s, size_t *idx, int c)
{
	if (s[*idx] != c)
		return false;

	(*idx)++;
	return true;
}

static qualifier_t _read_qualifier(const char *s, size_t *idx)
{
	switch (s[(*idx)++]) {
	case 't': /* ptrdiff_t */
	case 'z': /* size_t */
		if (sizeof(ptrdiff_t) == sizeof(int))
			return PrintfQualifierInt;
		else
			return PrintfQualifierLong;

	case 'h':
		if (_eat_char(s, idx, 'h'))
			return PrintfQualifierByte;
		else
			return PrintfQualifierShort;

	case 'l':
		if (_eat_char(s, idx, 'l'))
			return PrintfQualifierLongLong;
		else
			return PrintfQualifierLong;

	case 'j':
		return PrintfQualifierLongLong;

	default:
		--*idx;

		/* Unspecified */
		return PrintfQualifierInt;
	}
}

/** Print formatted string.
 *
 * Print string formatted according to the fmt parameter and variadic arguments.
 * Each formatting directive must have the following form:
 *
 *  \% [ FLAGS ] [ WIDTH ] [ .PRECISION ] [ TYPE ] CONVERSION
 *
 * FLAGS:@n
 *  - "#" Force to print prefix. For \%o conversion, the prefix is 0, for
 *        \%x and \%X prefixes are 0x and 0X and for conversion \%b the
 *        prefix is 0b.
 *
 *  - "-" Align to left.
 *
 *  - "+" Print positive sign just as negative.
 *
 *  - " " If the printed number is positive and "+" flag is not set,
 *        print space in place of sign.
 *
 *  - "0" Print 0 as padding instead of spaces. Zeroes are placed between
 *        sign and the rest of the number. This flag is ignored if "-"
 *        flag is specified.
 *
 * WIDTH:@n
 *  - Specify the minimal width of a printed argument. If it is bigger,
 *    width is ignored. If width is specified with a "*" character instead of
 *    number, width is taken from parameter list. And integer parameter is
 *    expected before parameter for processed conversion specification. If
 *    this value is negative its absolute value is taken and the "-" flag is
 *    set.
 *
 * PRECISION:@n
 *  - Value precision. For numbers it specifies minimum valid numbers.
 *    Smaller numbers are printed with leading zeroes. Bigger numbers are not
 *    affected. Strings with more than precision characters are cut off. Just
 *    as with width, an "*" can be used used instead of a number. An integer
 *    value is then expected in parameters. When both width and precision are
 *    specified using "*", the first parameter is used for width and the
 *    second one for precision.
 *
 * TYPE:@n
 *  - "hh" Signed or unsigned char.@n
 *  - "h"  Signed or unsigned short.@n
 *  - ""   Signed or unsigned int (default value).@n
 *  - "l"  Signed or unsigned long int.@n
 *         If conversion is "c", the character is wint_t (wide character).@n
 *         If conversion is "s", the string is char32_t * (wide string).@n
 *  - "ll" Signed or unsigned long long int.@n
 *  - "z"  Signed or unsigned ssize_t or site_t.@n
 *
 * CONVERSION:@n
 *  - % Print percentile character itself.
 *
 *  - c Print single character. The character is expected to be plain
 *      ASCII (e.g. only values 0 .. 127 are valid).@n
 *      If type is "l", then the character is expected to be wide character
 *      (e.g. values 0 .. 0x10ffff are valid).
 *
 *  - s Print zero terminated string. If a NULL value is passed as
 *      value, "(NULL)" is printed instead.@n
 *      If type is "l", then the string is expected to be wide string.
 *
 *  - P, p Print value of a pointer. Void * value is expected and it is
 *         printed in hexadecimal notation with prefix (as with
 *         \%#0.8X / \%#0.8x for 32-bit or \%#0.16lX / \%#0.16lx for 64-bit
 *         long pointers).
 *
 *  - b Print value as unsigned binary number. Prefix is not printed by
 *      default. (Nonstandard extension.)
 *
 *  - o Print value as unsigned octal number. Prefix is not printed by
 *      default.
 *
 *  - d, i Print signed decimal number. There is no difference between d
 *         and i conversion.
 *
 *  - u Print unsigned decimal number.
 *
 *  - X, x Print hexadecimal number with upper- or lower-case. Prefix is
 *         not printed by default.
 *
 * All other characters from fmt except the formatting directives are printed
 * verbatim.
 *
 * @param fmt Format NULL-terminated string.
 *
 * @return Number of characters printed, negative value on failure.
 *
 */
int printf_core(const char *fmt, printf_spec_t *ps, va_list ap)
{
	errno_t rc = EOK;
	size_t nxt = 0;  /* Index of the next character from fmt */

	size_t counter = 0;   /* Number of characters printed */

	while (rc == EOK) {
		/* Find the next specifier and write all the bytes before it. */
		const char *s = _strchrnul(&fmt[nxt], '%');
		size_t bytes = s - &fmt[nxt];
		rc = _write_bytes(&fmt[nxt], bytes, ps, &counter);
		if (rc != EOK)
			break;

		nxt += bytes;

		/* Check for end of string. */
		if (_eat_char(fmt, &nxt, 0))
			break;

		/* We must be at the start of a specifier. */
		bool spec = _eat_char(fmt, &nxt, '%');
		assert(spec);

		/* Parse modifiers */
		uint32_t flags = _parse_flags(fmt, &nxt);

		/* Width & '*' operator */
		int width = -1;
		if (_eat_char(fmt, &nxt, '*')) {
			/* Get width value from argument list */
			width = va_arg(ap, int);

			if (width < 0) {
				/* Negative width sets '-' flag */
				width = (width == INT_MIN) ? INT_MAX : -width;
				flags |= __PRINTF_FLAG_LEFTALIGNED;
			}
		} else {
			width = _read_num(fmt, &nxt);
		}

		/* Precision and '*' operator */
		int precision = -1;
		if (_eat_char(fmt, &nxt, '.')) {
			if (_eat_char(fmt, &nxt, '*')) {
				/* Get precision value from the argument list */
				precision = va_arg(ap, int);

				/* Negative is treated as omitted. */
				if (precision < 0)
					precision = -1;
			} else {
				precision = _read_num(fmt, &nxt);
			}
		}

		qualifier_t qualifier = _read_qualifier(fmt, &nxt);
		unsigned int base = 10;
		char specifier = fmt[nxt++];

		switch (specifier) {
		/*
		 * String and character conversions.
		 */
		case 's':
			if (qualifier == PrintfQualifierLong)
				rc = _format_wstr(va_arg(ap, char32_t *), width, precision, flags, ps, &counter);
			else
				rc = _format_cstr(va_arg(ap, char *), width, precision, flags, ps, &counter);
			continue;

		case 'c':
			if (qualifier == PrintfQualifierLong)
				rc = _format_uchar(va_arg(ap, wint_t), width, flags, ps, &counter);
			else
				rc = _format_char(va_arg(ap, int), width, flags, ps, &counter);
			continue;

		/*
		 * Floating point values
		 */
		case 'G':
		case 'g':
		case 'F':
		case 'f':
		case 'E':
		case 'e':;
#ifdef HAS_FLOAT
			rc = _format_double(va_arg(ap, double), specifier, precision,
			    width, flags, ps, &counter);
#else
			rc = _format_cstr("<float unsupported>", width, -1, 0, ps, &counter);
#endif
			continue;

		/*
		 * Integer values
		 */
		case 'P':
			/* Pointer */
			flags |= __PRINTF_FLAG_BIGCHARS;
			/* Fallthrough */
		case 'p':
			flags |= __PRINTF_FLAG_PREFIX;
			flags |= __PRINTF_FLAG_ZEROPADDED;
			base = 16;
			qualifier = PrintfQualifierPointer;
			break;
		case 'b':
			base = 2;
			break;
		case 'o':
			base = 8;
			break;
		case 'd':
		case 'i':
			flags |= __PRINTF_FLAG_SIGNED;
			break;
		case 'u':
			break;
		case 'X':
			flags |= __PRINTF_FLAG_BIGCHARS;
			/* Fallthrough */
		case 'x':
			base = 16;
			break;

		case '%':
			/* Percentile itself */
			rc = _write_char('%', ps, &counter);
			continue;

		/*
		 * Bad formatting.
		 */
		default:
			rc = EINVAL;
			continue;
		}

		/* Print integers */
		uint64_t number;

		switch (qualifier) {
		case PrintfQualifierByte:
			number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
			break;
		case PrintfQualifierShort:
			number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
			break;
		case PrintfQualifierInt:
			number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
			break;
		case PrintfQualifierLong:
			number = PRINTF_GET_INT_ARGUMENT(long, ap, flags);
			break;
		case PrintfQualifierLongLong:
			number = PRINTF_GET_INT_ARGUMENT(long long, ap, flags);
			break;
		case PrintfQualifierPointer:
			precision = sizeof(void *) << 1;
			number = (uint64_t) (uintptr_t) va_arg(ap, void *);
			break;
		default:
			/* Unknown qualifier */
			rc = EINVAL;
			continue;
		}

		rc = _format_number(number, width, precision, base, flags, ps, &counter);
	}

	if (rc != EOK) {
		_set_errno(rc);
		return -1;
	}

	if (counter > INT_MAX) {
		_set_errno(EOVERFLOW);
		return -1;
	}

	return (int) counter;
}

/** @}
 */
