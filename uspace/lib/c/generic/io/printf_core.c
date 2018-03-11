/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup generic
 * @{
 */
/**
 * @file
 * @brief Printing functions.
 */

#include <stdio.h>
#include <stddef.h>
#include <io/printf_core.h>
#include <ctype.h>
#include <str.h>
#include <double_to_str.h>
#include <ieee_double.h>
#include <assert.h>
#include <macros.h>
#include <wchar.h>


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
 * Buffer big enough for 64-bit number printed in base 2, sign, prefix and 0
 * to terminate string... (last one is only for better testing end of buffer by
 * zero-filling subroutine)
 */
#define PRINT_NUMBER_BUFFER_SIZE  (64 + 5)

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
	PrintfQualifierSize,
	PrintfQualifierMax
} qualifier_t;

static const char *nullstr = "(NULL)";
static const char *digits_small = "0123456789abcdef";
static const char *digits_big = "0123456789ABCDEF";
static const char invalch = U_SPECIAL;



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
static int get_sign_char(bool negative, uint32_t flags)
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

/** Prints count times character ch. */
static int print_padding(char ch, int count, printf_spec_t *ps)
{
	for (int i = 0; i < count; ++i) {
		if (ps->str_write(&ch, 1, ps->data) < 0) {
			return -1;
		}
	}

	return count;
}


/** Print one or more characters without adding newline.
 *
 * @param buf  Buffer holding characters with size of
 *             at least size bytes. NULL is not allowed!
 * @param size Size of the buffer in bytes.
 * @param ps   Output method and its data.
 *
 * @return Number of characters printed.
 *
 */
static int printf_putnchars(const char *buf, size_t size,
    printf_spec_t *ps)
{
	return ps->str_write((void *) buf, size, ps->data);
}

/** Print one or more wide characters without adding newline.
 *
 * @param buf  Buffer holding wide characters with size of
 *             at least size bytes. NULL is not allowed!
 * @param size Size of the buffer in bytes.
 * @param ps   Output method and its data.
 *
 * @return Number of wide characters printed.
 *
 */
static int printf_wputnchars(const wchar_t *buf, size_t size,
    printf_spec_t *ps)
{
	return ps->wstr_write((void *) buf, size, ps->data);
}

/** Print string without adding a newline.
 *
 * @param str String to print.
 * @param ps  Write function specification and support data.
 *
 * @return Number of characters printed.
 *
 */
static int printf_putstr(const char *str, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putnchars(nullstr, str_size(nullstr), ps);

	return ps->str_write((void *) str, str_size(str), ps->data);
}

/** Print one ASCII character.
 *
 * @param c  ASCII character to be printed.
 * @param ps Output method.
 *
 * @return Number of characters printed.
 *
 */
static int printf_putchar(const char ch, printf_spec_t *ps)
{
	if (!ascii_check(ch))
		return ps->str_write((void *) &invalch, 1, ps->data);

	return ps->str_write(&ch, 1, ps->data);
}

/** Print one wide character.
 *
 * @param c  Wide character to be printed.
 * @param ps Output method.
 *
 * @return Number of characters printed.
 *
 */
static int printf_putwchar(const wchar_t ch, printf_spec_t *ps)
{
	if (!chr_check(ch))
		return ps->str_write((void *) &invalch, 1, ps->data);

	return ps->wstr_write(&ch, sizeof(wchar_t), ps->data);
}

/** Print one formatted ASCII character.
 *
 * @param ch    Character to print.
 * @param width Width modifier.
 * @param flags Flags that change the way the character is printed.
 *
 * @return Number of characters printed, negative value on failure.
 *
 */
static int print_char(const char ch, int width, uint32_t flags, printf_spec_t *ps)
{
	size_t counter = 0;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (--width > 0) {
			/*
			 * One space is consumed by the character itself, hence
			 * the predecrement.
			 */
			if (printf_putchar(' ', ps) > 0)
				counter++;
		}
	}

	if (printf_putchar(ch, ps) > 0)
		counter++;

	while (--width > 0) {
		/*
		 * One space is consumed by the character itself, hence
		 * the predecrement.
		 */
		if (printf_putchar(' ', ps) > 0)
			counter++;
	}

	return (int) (counter);
}

/** Print one formatted wide character.
 *
 * @param ch    Character to print.
 * @param width Width modifier.
 * @param flags Flags that change the way the character is printed.
 *
 * @return Number of characters printed, negative value on failure.
 *
 */
static int print_wchar(const wchar_t ch, int width, uint32_t flags, printf_spec_t *ps)
{
	size_t counter = 0;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (--width > 0) {
			/*
			 * One space is consumed by the character itself, hence
			 * the predecrement.
			 */
			if (printf_putchar(' ', ps) > 0)
				counter++;
		}
	}

	if (printf_putwchar(ch, ps) > 0)
		counter++;

	while (--width > 0) {
		/*
		 * One space is consumed by the character itself, hence
		 * the predecrement.
		 */
		if (printf_putchar(' ', ps) > 0)
			counter++;
	}

	return (int) (counter);
}

/** Print string.
 *
 * @param str       String to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 *
 * @return Number of characters printed, negative value on failure.
 */
static int print_str(char *str, int width, unsigned int precision,
    uint32_t flags, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putstr(nullstr, ps);

	size_t strw = str_length(str);

	/* Precision unspecified - print everything. */
	if ((precision == 0) || (precision > strw))
		precision = strw;

	/* Left padding */
	size_t counter = 0;
	width -= precision;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) {
			if (printf_putchar(' ', ps) == 1)
				counter++;
		}
	}

	/* Part of @a str fitting into the alloted space. */
	int retval;
	size_t size = str_lsize(str, precision);
	if ((retval = printf_putnchars(str, size, ps)) < 0)
		return -counter;

	counter += retval;

	/* Right padding */
	while (width-- > 0) {
		if (printf_putchar(' ', ps) == 1)
			counter++;
	}

	return ((int) counter);

}

/** Print wide string.
 *
 * @param str       Wide string to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 *
 * @return Number of wide characters printed, negative value on failure.
 */
static int print_wstr(wchar_t *str, int width, unsigned int precision,
    uint32_t flags, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putstr(nullstr, ps);

	size_t strw = wstr_length(str);

	/* Precision not specified - print everything. */
	if ((precision == 0) || (precision > strw))
		precision = strw;

	/* Left padding */
	size_t counter = 0;
	width -= precision;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) {
			if (printf_putchar(' ', ps) == 1)
				counter++;
		}
	}

	/* Part of @a wstr fitting into the alloted space. */
	int retval;
	size_t size = wstr_lsize(str, precision);
	if ((retval = printf_wputnchars(str, size, ps)) < 0)
		return -counter;

	counter += retval;

	/* Right padding */
	while (width-- > 0) {
		if (printf_putchar(' ', ps) == 1)
			counter++;
	}

	return ((int) counter);
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
 *
 * @return Number of characters printed.
 *
 */
static int print_number(uint64_t num, int width, int precision, int base,
    uint32_t flags, printf_spec_t *ps)
{
	/* Precision not specified. */
	if (precision < 0) {
		precision = 0;
	}

	const char *digits;
	if (flags & __PRINTF_FLAG_BIGCHARS)
		digits = digits_big;
	else
		digits = digits_small;

	char data[PRINT_NUMBER_BUFFER_SIZE];
	char *ptr = &data[PRINT_NUMBER_BUFFER_SIZE - 1];

	/* Size of number with all prefixes and signs */
	int size = 0;

	/* Put zero at end of string */
	*ptr-- = 0;

	if (num == 0) {
		*ptr-- = '0';
		size++;
	} else {
		do {
			*ptr-- = digits[num % base];
			size++;
		} while (num /= base);
	}

	/* Size of plain number */
	int number_size = size;

	/*
	 * Collect the sum of all prefixes/signs/etc. to calculate padding and
	 * leading zeroes.
	 */
	if (flags & __PRINTF_FLAG_PREFIX) {
		switch (base) {
		case 2:
			/* Binary formating is not standard, but usefull */
			size += 2;
			break;
		case 8:
			size++;
			break;
		case 16:
			size += 2;
			break;
		}
	}

	char sgn = 0;
	if (flags & __PRINTF_FLAG_SIGNED) {
		if (flags & __PRINTF_FLAG_NEGATIVE) {
			sgn = '-';
			size++;
		} else if (flags & __PRINTF_FLAG_SHOWPLUS) {
			sgn = '+';
			size++;
		} else if (flags & __PRINTF_FLAG_SPACESIGN) {
			sgn = ' ';
			size++;
		}
	}

	if (flags & __PRINTF_FLAG_LEFTALIGNED)
		flags &= ~__PRINTF_FLAG_ZEROPADDED;

	/*
	 * If the number is left-aligned or precision is specified then
	 * padding with zeros is ignored.
	 */
	if (flags & __PRINTF_FLAG_ZEROPADDED) {
		if ((precision == 0) && (width > size))
			precision = width - size + number_size;
	}

	/* Print leading spaces */
	if (number_size > precision) {
		/* Print the whole number, not only a part */
		precision = number_size;
	}

	width -= precision + size - number_size;
	size_t counter = 0;

	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) {
			if (printf_putchar(' ', ps) == 1)
				counter++;
		}
	}

	/* Print sign */
	if (sgn) {
		if (printf_putchar(sgn, ps) == 1)
			counter++;
	}

	/* Print prefix */
	if (flags & __PRINTF_FLAG_PREFIX) {
		switch (base) {
		case 2:
			/* Binary formating is not standard, but usefull */
			if (printf_putchar('0', ps) == 1)
				counter++;
			if (flags & __PRINTF_FLAG_BIGCHARS) {
				if (printf_putchar('B', ps) == 1)
					counter++;
			} else {
				if (printf_putchar('b', ps) == 1)
					counter++;
			}
			break;
		case 8:
			if (printf_putchar('o', ps) == 1)
				counter++;
			break;
		case 16:
			if (printf_putchar('0', ps) == 1)
				counter++;
			if (flags & __PRINTF_FLAG_BIGCHARS) {
				if (printf_putchar('X', ps) == 1)
					counter++;
			} else {
				if (printf_putchar('x', ps) == 1)
					counter++;
			}
			break;
		}
	}

	/* Print leading zeroes */
	precision -= number_size;
	while (precision-- > 0) {
		if (printf_putchar('0', ps) == 1)
			counter++;
	}

	/* Print the number itself */
	int retval;
	if ((retval = printf_putstr(++ptr, ps)) > 0)
		counter += retval;

	/* Print trailing spaces */

	while (width-- > 0) {
		if (printf_putchar(' ', ps) == 1)
			counter++;
	}

	return ((int) counter);
}

/** Prints a special double (ie NaN, infinity) padded to width characters. */
static int print_special(ieee_double_t val, int width, uint32_t flags,
	printf_spec_t *ps)
{
	assert(val.is_special);

	char sign = get_sign_char(val.is_negative, flags);

	const int str_len = 3;
	const char *str;

	if (flags & __PRINTF_FLAG_BIGCHARS) {
		str = val.is_infinity ? "INF" : "NAN";
	} else {
		str = val.is_infinity ? "inf" : "nan";
	}

	int padding_len = max(0, width - ((sign ? 1 : 0) + str_len));

	int counter = 0;
	int ret;

	/* Leading padding. */
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	if (sign) {
		if ((ret = ps->str_write(&sign, 1, ps->data)) < 0)
			return -1;

		counter += ret;
	}

	if ((ret = ps->str_write(str, str_len, ps->data)) < 0)
		return -1;

	counter += ret;


	/* Trailing padding. */
	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	return counter;
}

/** Trims trailing zeros but leaves a single "0" intact. */
static void fp_trim_trailing_zeros(char *buf, int *len, int *dec_exp)
{
	/* Cut the zero off by adjusting the exponent. */
	while (2 <= *len && '0' == buf[*len - 1]) {
		--*len;
		++*dec_exp;
	}
}

/** Textually round up the last digit thereby eliminating it. */
static void fp_round_up(char *buf, int *len, int *dec_exp)
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
static int print_double_str_fixed(double_str_t *val_str, int precision, int width,
	uint32_t flags, printf_spec_t *ps)
{
	int len = val_str->len;
	char *buf = val_str->str;
	int dec_exp = val_str->dec_exp;

	assert(0 < len);
	assert(0 <= precision);
	assert(0 <= dec_exp || -dec_exp <= precision);

	/* Number of integral digits to print (at least leading zero). */
	int int_len = max(1, len + dec_exp);

	char sign = get_sign_char(val_str->neg, flags);

	/* Fractional portion lengths. */
	int last_frac_signif_pos = max(0, -dec_exp);
	int leading_frac_zeros = max(0, last_frac_signif_pos - len);
	int signif_frac_figs = min(last_frac_signif_pos, len);
	int trailing_frac_zeros = precision - last_frac_signif_pos;
	char *buf_frac = buf + len - signif_frac_figs;

	if (flags & __PRINTF_FLAG_NOFRACZEROS) {
		trailing_frac_zeros = 0;
	}

	int frac_len = leading_frac_zeros + signif_frac_figs + trailing_frac_zeros;

	bool has_decimal_pt = (0 < frac_len) || (flags & __PRINTF_FLAG_DECIMALPT);

	/* Number of non-padding chars to print. */
	int num_len = (sign ? 1 : 0) + int_len + (has_decimal_pt ? 1 : 0) + frac_len;

	int padding_len = max(0, width - num_len);
	int ret = 0;
	int counter = 0;

	/* Leading padding and sign. */

	if (!(flags & (__PRINTF_FLAG_LEFTALIGNED | __PRINTF_FLAG_ZEROPADDED))) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	if (sign) {
		if ((ret = ps->str_write(&sign, 1, ps->data)) < 0)
			return -1;

		counter += ret;
	}

	if (flags & __PRINTF_FLAG_ZEROPADDED) {
		if ((ret = print_padding('0', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	/* Print the intergral part of the buffer. */

	int buf_int_len = min(len, len + dec_exp);

	if (0 < buf_int_len) {
		if ((ret = ps->str_write(buf, buf_int_len, ps->data)) < 0)
			return -1;

		counter += ret;

		/* Print trailing zeros of the integral part of the number. */
		if ((ret = print_padding('0', int_len - buf_int_len, ps)) < 0)
			return -1;
	} else {
		/* Single leading integer 0. */
		char ch = '0';
		if ((ret = ps->str_write(&ch, 1, ps->data)) < 0)
			return -1;
	}

	counter += ret;

	/* Print the decimal point and the fractional part. */
	if (has_decimal_pt) {
		char ch = '.';

		if ((ret = ps->str_write(&ch, 1, ps->data)) < 0)
			return -1;

		counter += ret;

		/* Print leading zeros of the fractional part of the number. */
		if ((ret = print_padding('0', leading_frac_zeros, ps)) < 0)
			return -1;

		counter += ret;

		/* Print significant digits of the fractional part of the number. */
		if (0 < signif_frac_figs) {
			if ((ret = ps->str_write(buf_frac, signif_frac_figs, ps->data)) < 0)
				return -1;

			counter += ret;
		}

		/* Print trailing zeros of the fractional part of the number. */
		if ((ret = print_padding('0', trailing_frac_zeros, ps)) < 0)
			return -1;

		counter += ret;
	}

	/* Trailing padding. */
	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	return counter;
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
 *
 * @return The number of characters printed; negative on failure.
 */
static int print_double_fixed(double g, int precision, int width, uint32_t flags,
	printf_spec_t *ps)
{
	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		flags &= ~__PRINTF_FLAG_ZEROPADDED;
	}

	if (flags & __PRINTF_FLAG_DECIMALPT) {
		flags &= ~__PRINTF_FLAG_NOFRACZEROS;
	}

	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special) {
		return print_special(val, width, flags, ps);
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
		fp_round_up(buf, &val_str.len, &val_str.dec_exp);

		/* Rounding could have introduced trailing zeros. */
		if (flags & __PRINTF_FLAG_NOFRACZEROS) {
			fp_trim_trailing_zeros(buf, &val_str.len, &val_str.dec_exp);
		}
	} else {
		/* Let the implementation figure out the proper precision. */
		val_str.len = double_to_short_str(val, buf, buf_size, &val_str.dec_exp);

		/* Precision needed for the last significant digit. */
		precision = max(0, -val_str.dec_exp);
	}

	return print_double_str_fixed(&val_str, precision, width, flags, ps);
}

/** Prints the decimal exponent part of a %e specifier formatted number. */
static int print_exponent(int exp_val, uint32_t flags, printf_spec_t *ps)
{
	int counter = 0;
	int ret;

	char exp_ch = (flags & __PRINTF_FLAG_BIGCHARS) ? 'E' : 'e';

	if ((ret = ps->str_write(&exp_ch, 1, ps->data)) < 0)
		return -1;

	counter += ret;

	char exp_sign = (exp_val < 0) ? '-' : '+';

	if ((ret = ps->str_write(&exp_sign, 1, ps->data)) < 0)
		return -1;

	counter += ret;

	/* Print the exponent. */
	exp_val = abs(exp_val);

	char exp_str[4] = { 0 };

	exp_str[0] = '0' + exp_val / 100;
	exp_str[1] = '0' + (exp_val % 100) / 10 ;
	exp_str[2] = '0' + (exp_val % 10);

	int exp_len = (exp_str[0] == '0') ? 2 : 3;
	const char *exp_str_start = &exp_str[3] - exp_len;

	if ((ret = ps->str_write(exp_str_start, exp_len, ps->data)) < 0)
		return -1;

	counter += ret;

	return counter;
}


/** Format and print the double string repressentation according
 *  to the %e specifier.
 */
static int print_double_str_scient(double_str_t *val_str, int precision,
	int width, uint32_t flags, printf_spec_t *ps)
{
	int len = val_str->len;
	int dec_exp = val_str->dec_exp;
	char *buf  = val_str->str;

	assert(0 < len);

	char sign = get_sign_char(val_str->neg, flags);
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
	int ret = 0;
	int counter = 0;

	if (!(flags & (__PRINTF_FLAG_LEFTALIGNED | __PRINTF_FLAG_ZEROPADDED))) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	if (sign) {
		if ((ret = ps->str_write(&sign, 1, ps->data)) < 0)
			return -1;

		counter += ret;
	}

	if (flags & __PRINTF_FLAG_ZEROPADDED) {
		if ((ret = print_padding('0', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	/* Single leading integer. */
	if ((ret = ps->str_write(buf, 1, ps->data)) < 0)
		return -1;

	counter += ret;

	/* Print the decimal point and the fractional part. */
	if (has_decimal_pt) {
		char ch = '.';

		if ((ret = ps->str_write(&ch, 1, ps->data)) < 0)
			return -1;

		counter += ret;

		/* Print significant digits of the fractional part of the number. */
		if (0 < signif_frac_figs) {
			if ((ret = ps->str_write(buf + 1, signif_frac_figs, ps->data)) < 0)
				return -1;

			counter += ret;
		}

		/* Print trailing zeros of the fractional part of the number. */
		if ((ret = print_padding('0', trailing_frac_zeros, ps)) < 0)
			return -1;

		counter += ret;
	}

	/* Print the exponent. */
	if ((ret = print_exponent(exp_val, flags, ps)) < 0)
		return -1;

	counter += ret;

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		if ((ret = print_padding(' ', padding_len, ps)) < 0)
			return -1;

		counter += ret;
	}

	return counter;
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
 *
 * @return The number of characters printed; negative on failure.
 */
static int print_double_scientific(double g, int precision, int width,
	uint32_t flags, printf_spec_t *ps)
{
	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		flags &= ~__PRINTF_FLAG_ZEROPADDED;
	}

	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special) {
		return print_special(val, width, flags, ps);
	}

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
		fp_round_up(buf, &val_str.len, &val_str.dec_exp);

		/* Rounding could have introduced trailing zeros. */
		if (flags & __PRINTF_FLAG_NOFRACZEROS) {
			fp_trim_trailing_zeros(buf, &val_str.len, &val_str.dec_exp);
		}
	} else {
		/* Let the implementation figure out the proper precision. */
		val_str.len = double_to_short_str(val, buf, buf_size, &val_str.dec_exp);

		/* Use all produced digits. */
		precision = val_str.len - 1;
	}

	return print_double_str_scient(&val_str, precision, width, flags, ps);
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
static int print_double_generic(double g, int precision, int width,
	uint32_t flags, printf_spec_t *ps)
{
	ieee_double_t val = extract_ieee_double(g);

	if (val.is_special) {
		return print_special(val, width, flags, ps);
	}

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
			return print_double_fixed(g, precision, width,
				flags | __PRINTF_FLAG_NOFRACZEROS, ps);
		} else {
			--precision;
			return print_double_scientific(g, precision, width,
				flags | __PRINTF_FLAG_NOFRACZEROS, ps);
		}
	} else {
		/* Convert to get the decimal exponent and digit count.*/
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
			return print_double_str_fixed(&val_str, precision, width, flags, ps);
		} else {
			/* Use all produced digits. */
			precision = val_str.len - 1;
			return print_double_str_scient(&val_str, precision, width, flags, ps);
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
 *
 * @return The number of characters printed; negative on failure.
 */
static int print_double(double g, char spec, int precision, int width,
	uint32_t flags, printf_spec_t *ps)
{
	switch (spec) {
	case 'F':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'f':
		precision = (precision < 0) ? 6 : precision;
		return print_double_fixed(g, precision, width, flags, ps);

	case 'E':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'e':
		precision = (precision < 0) ? 6 : precision;
		return print_double_scientific(g, precision, width, flags, ps);

	case 'G':
		flags |= __PRINTF_FLAG_BIGCHARS;
		/* Fallthrough */
	case 'g':
		return print_double_generic(g, precision, width, flags, ps);

	default:
		assert(false);
		return -1;
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
 *         If conversion is "s", the string is wchar_t * (wide string).@n
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
	size_t i;        /* Index of the currently processed character from fmt */
	size_t nxt = 0;  /* Index of the next character from fmt */
	size_t j = 0;    /* Index to the first not printed nonformating character */

	size_t counter = 0;   /* Number of characters printed */
	int retval;           /* Return values from nested functions */

	while (true) {
		i = nxt;
		wchar_t uc = str_decode(fmt, &nxt, STR_NO_LIMIT);

		if (uc == 0)
			break;

		/* Control character */
		if (uc == '%') {
			/* Print common characters if any processed */
			if (i > j) {
				if ((retval = printf_putnchars(&fmt[j], i - j, ps)) < 0) {
					/* Error */
					counter = -counter;
					goto out;
				}
				counter += retval;
			}

			j = i;

			/* Parse modifiers */
			uint32_t flags = 0;
			bool end = false;

			do {
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				switch (uc) {
				case '#':
					flags |= __PRINTF_FLAG_PREFIX;
					flags |= __PRINTF_FLAG_DECIMALPT;
					break;
				case '-':
					flags |= __PRINTF_FLAG_LEFTALIGNED;
					break;
				case '+':
					flags |= __PRINTF_FLAG_SHOWPLUS;
					break;
				case ' ':
					flags |= __PRINTF_FLAG_SPACESIGN;
					break;
				case '0':
					flags |= __PRINTF_FLAG_ZEROPADDED;
					break;
				default:
					end = true;
				}
			} while (!end);

			/* Width & '*' operator */
			int width = 0;
			if (isdigit(uc)) {
				while (true) {
					width *= 10;
					width += uc - '0';

					i = nxt;
					uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
					if (uc == 0)
						break;
					if (!isdigit(uc))
						break;
				}
			} else if (uc == '*') {
				/* Get width value from argument list */
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				width = (int) va_arg(ap, int);
				if (width < 0) {
					/* Negative width sets '-' flag */
					width *= -1;
					flags |= __PRINTF_FLAG_LEFTALIGNED;
				}
			}

			/* Precision and '*' operator */
			int precision = -1;
			if (uc == '.') {
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				if (isdigit(uc)) {
					precision = 0;
					while (true) {
						precision *= 10;
						precision += uc - '0';

						i = nxt;
						uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
						if (uc == 0)
							break;
						if (!isdigit(uc))
							break;
					}
				} else if (uc == '*') {
					/* Get precision value from the argument list */
					i = nxt;
					uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
					precision = (int) va_arg(ap, int);
					if (precision < 0) {
						/* Ignore negative precision - use default instead */
						precision = -1;
					}
				}
			}

			qualifier_t qualifier;

			switch (uc) {
			case 't':
				/* ptrdiff_t */
				if (sizeof(ptrdiff_t) == sizeof(int32_t))
					qualifier = PrintfQualifierInt;
				else
					qualifier = PrintfQualifierLongLong;
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				break;
			case 'h':
				/* Char or short */
				qualifier = PrintfQualifierShort;
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				if (uc == 'h') {
					i = nxt;
					uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
					qualifier = PrintfQualifierByte;
				}
				break;
			case 'l':
				/* Long or long long */
				qualifier = PrintfQualifierLong;
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				if (uc == 'l') {
					i = nxt;
					uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
					qualifier = PrintfQualifierLongLong;
				}
				break;
			case 'z':
				qualifier = PrintfQualifierSize;
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				break;
			case 'j':
				qualifier = PrintfQualifierMax;
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				break;
			default:
				/* Default type */
				qualifier = PrintfQualifierInt;
			}

			unsigned int base = 10;

			switch (uc) {
			/*
			 * String and character conversions.
			 */
			case 's':
				precision = max(0,  precision);

				if (qualifier == PrintfQualifierLong)
					retval = print_wstr(va_arg(ap, wchar_t *), width, precision, flags, ps);
				else
					retval = print_str(va_arg(ap, char *), width, precision, flags, ps);

				if (retval < 0) {
					counter = -counter;
					goto out;
				}

				counter += retval;
				j = nxt;
				continue;
			case 'c':
				if (qualifier == PrintfQualifierLong)
					retval = print_wchar(va_arg(ap, wint_t), width, flags, ps);
				else
					retval = print_char(va_arg(ap, unsigned int), width, flags, ps);

				if (retval < 0) {
					counter = -counter;
					goto out;
				}

				counter += retval;
				j = nxt;
				continue;

			/*
			 * Floating point values
			 */
			case 'G':
			case 'g':
			case 'F':
			case 'f':
			case 'E':
			case 'e':
				retval = print_double(va_arg(ap, double), uc, precision,
					width, flags, ps);

				if (retval < 0) {
					counter = -counter;
					goto out;
				}

				counter += retval;
				j = nxt;
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
				/* Fallthrough */
			case 'u':
				break;
			case 'X':
				flags |= __PRINTF_FLAG_BIGCHARS;
				/* Fallthrough */
			case 'x':
				base = 16;
				break;

			/* Percentile itself */
			case '%':
				j = i;
				continue;

			/*
			 * Bad formatting.
			 */
			default:
				/*
				 * Unknown format. Now, j is the index of '%'
				 * so we will print whole bad format sequence.
				 */
				continue;
			}

			/* Print integers */
			size_t size;
			uint64_t number;

			switch (qualifier) {
			case PrintfQualifierByte:
				size = sizeof(unsigned char);
				number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
				break;
			case PrintfQualifierShort:
				size = sizeof(unsigned short);
				number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
				break;
			case PrintfQualifierInt:
				size = sizeof(unsigned int);
				number = PRINTF_GET_INT_ARGUMENT(int, ap, flags);
				break;
			case PrintfQualifierLong:
				size = sizeof(unsigned long);
				number = PRINTF_GET_INT_ARGUMENT(long, ap, flags);
				break;
			case PrintfQualifierLongLong:
				size = sizeof(unsigned long long);
				number = PRINTF_GET_INT_ARGUMENT(long long, ap, flags);
				break;
			case PrintfQualifierPointer:
				size = sizeof(void *);
				precision = size << 1;
				number = (uint64_t) (uintptr_t) va_arg(ap, void *);
				break;
			case PrintfQualifierSize:
				size = sizeof(size_t);
				number = (uint64_t) va_arg(ap, size_t);
				break;
			case PrintfQualifierMax:
				size = sizeof(uintmax_t);
				number = (uint64_t) va_arg(ap, uintmax_t);
				break;
			default:
				/* Unknown qualifier */
				counter = -counter;
				goto out;
			}

			if ((retval = print_number(number, width, precision,
			    base, flags, ps)) < 0) {
				counter = -counter;
				goto out;
			}

			counter += retval;
			j = nxt;
		}
	}

	if (i > j) {
		if ((retval = printf_putnchars(&fmt[j], i - j, ps)) < 0) {
			/* Error */
			counter = -counter;
			goto out;
		}
		counter += retval;
	}

out:
	return ((int) counter);
}

/** @}
 */
