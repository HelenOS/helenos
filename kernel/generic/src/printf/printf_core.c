/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/**
 * @file
 * @brief Printing functions.
 */

#include <printf/printf_core.h>
#include <print.h>
#include <stdarg.h>
#include <macros.h>
#include <stddef.h>
#include <str.h>
#include <arch.h>

/** show prefixes 0x or 0 */
#define __PRINTF_FLAG_PREFIX       0x00000001

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
static int printf_wputnchars(const char32_t *buf, size_t size,
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
static int printf_putuchar(const char32_t ch, printf_spec_t *ps)
{
	if (!chr_check(ch))
		return ps->str_write((void *) &invalch, 1, ps->data);

	return ps->wstr_write(&ch, sizeof(char32_t), ps->data);
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
static int print_wchar(const char32_t ch, int width, uint32_t flags, printf_spec_t *ps)
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

	if (printf_putuchar(ch, ps) > 0)
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

	/* Print leading spaces. */
	size_t strw = str_length(str);
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
static int print_wstr(char32_t *str, int width, unsigned int precision,
    uint32_t flags, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putstr(nullstr, ps);

	/* Print leading spaces. */
	size_t strw = wstr_length(str);
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
 *         If conversion is "s", the string is char32_t * (UTF-32 string).@n
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
		char32_t uc = str_decode(fmt, &nxt, STR_NO_LIMIT);

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
			int precision = 0;
			if (uc == '.') {
				i = nxt;
				uc = str_decode(fmt, &nxt, STR_NO_LIMIT);
				if (isdigit(uc)) {
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
						/* Ignore negative precision */
						precision = 0;
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
				if (qualifier == PrintfQualifierLong)
					retval = print_wstr(va_arg(ap, char32_t *), width, precision, flags, ps);
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

			case '%':
				/* Percentile itself */
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
