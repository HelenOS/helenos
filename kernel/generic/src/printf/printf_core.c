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

#include <printf/printf_core.h>
#include <print.h>
#include <arch/arg.h>
#include <macros.h>
#include <string.h>
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

/** Enumeration of possible arguments types.
 */
typedef enum {
	PrintfQualifierByte = 0,
	PrintfQualifierShort,
	PrintfQualifierInt,
	PrintfQualifierLong,
	PrintfQualifierLongLong,
	PrintfQualifierPointer
} qualifier_t;

static char nullstr[] = "(NULL)";
static char digits_small[] = "0123456789abcdef";
static char digits_big[] = "0123456789ABCDEF";

/** Print one or more UTF-8 characters without adding newline.
 *
 * @param buf  Buffer holding UTF-8 characters with size of
 *             at least size bytes. NULL is not allowed!
 * @param size Size of the buffer in bytes.
 * @param ps   Output method and its data.
 *
 * @return Number of UTF-8 characters printed.
 *
 */
static int printf_putnchars_utf8(const char *buf, size_t size,
    printf_spec_t *ps)
{
	return ps->write_utf8((void *) buf, size, ps->data);
}

/** Print one or more UTF-32 characters without adding newline.
 *
 * @param buf  Buffer holding UTF-32 characters with size of
 *             at least size bytes. NULL is not allowed!
 * @param size Size of the buffer in bytes.
 * @param ps   Output method and its data.
 *
 * @return Number of UTF-32 characters printed.
 *
 */
static int printf_putnchars_utf32(const wchar_t *buf, size_t size,
    printf_spec_t *ps)
{
	return ps->write_utf32((void *) buf, size, ps->data);
}

/** Print UTF-8 string without adding a newline.
 *
 * @param str UTF-8 string to print.
 * @param ps  Write function specification and support data.
 *
 * @return Number of UTF-8 characters printed.
 *
 */
static int printf_putstr(const char *str, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putnchars_utf8(nullstr, strlen(nullstr), ps);
	
	return ps->write_utf8((void *) str, strlen(str), ps->data);
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
		return ps->write_utf8((void *) &invalch, 1, ps->data);
	
	return ps->write_utf8(&ch, 1, ps->data);
}

/** Print one UTF-32 character.
 *
 * @param c  UTF-32 character to be printed.
 * @param ps Output method.
 *
 * @return Number of characters printed.
 *
 */
static int printf_putwchar(const wchar_t ch, printf_spec_t *ps)
{
	if (!unicode_check(ch))
		return ps->write_utf8((void *) &invalch, 1, ps->data);
	
	return ps->write_utf32(&ch, 1, ps->data);
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
	count_t counter = 0;
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
	
	return (int) (counter + 1);
}

/** Print one formatted UTF-32 character.
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
	count_t counter = 0;
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
	
	return (int) (counter + 1);
}

/** Print UTF-8 string.
 *
 * @param str       UTF-8 string to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 *
 * @return Number of UTF-8 characters printed, negative value on failure.
 */
static int print_utf8(char *str, int width, unsigned int precision,
	uint32_t flags, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putstr(nullstr, ps);
	
	/* Print leading spaces */
	size_t size = strlen_utf8(str);
	if (precision == 0)
		precision = size;
	
	count_t counter = 0;
	width -= precision;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) {
			if (printf_putchar(' ', ps) == 1)
				counter++;
		}
	}
	
	int retval;
	size_t bytes = utf8_count_bytes(str, min(size, precision));
	if ((retval = printf_putnchars_utf8(str, bytes, ps)) < 0)
		return -counter;
	
	counter += retval;
	
	while (width-- > 0) {
		if (printf_putchar(' ', ps) == 1)
			counter++;
	}
	
	return ((int) counter);
}

/** Print UTF-32 string.
 *
 * @param str       UTF-32 string to be printed.
 * @param width     Width modifier.
 * @param precision Precision modifier.
 * @param flags     Flags that modify the way the string is printed.
 *
 * @return Number of UTF-32 characters printed, negative value on failure.
 */
static int print_utf32(wchar_t *str, int width, unsigned int precision,
	uint32_t flags, printf_spec_t *ps)
{
	if (str == NULL)
		return printf_putstr(nullstr, ps);
	
	/* Print leading spaces */
	size_t size = strlen_utf32(str);
	if (precision == 0)
		precision = size;
	
	count_t counter = 0;
	width -= precision;
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) {
			if (printf_putchar(' ', ps) == 1)
				counter++;
		}
	}
	
	int retval;
	size_t bytes = min(size, precision) * sizeof(wchar_t);
	if ((retval = printf_putnchars_utf32(str, bytes, ps)) < 0)
		return -counter;
	
	counter += retval;
	
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
	char *digits;
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
		switch(base) {
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
	count_t counter = 0;
	
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
		switch(base) {
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
	
	/* Print tailing spaces */
	
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
 *         If conversion is "c", the character is wchar_t (UTF-32).@n
 *         If conversion is "s", the string is wchar_t * (UTF-32).@n
 *  - "ll" Signed or unsigned long long int.@n
 *
 * CONVERSION:@n
 *  - % Print percentile character itself.
 *
 *  - c Print single character. The character is expected to be plain
 *      ASCII (e.g. only values 0 .. 127 are valid).@n
 *      If type is "l", then the character is expected to be UTF-32
 *      (e.g. values 0 .. 0x10ffff are valid).
 *
 *  - s Print zero terminated string. If a NULL value is passed as
 *      value, "(NULL)" is printed instead.@n
 *      The string is expected to be correctly encoded UTF-8 (or plain
 *      ASCII, which is a subset of UTF-8).@n
 *      If type is "l", then the string is expected to be correctly
 *      encoded UTF-32.
 *
 *  - P, p Print value of a pointer. Void * value is expected and it is
 *         printed in hexadecimal notation with prefix (as with \%#X / \%#x
 *         for 32-bit or \%#X / \%#x for 64-bit long pointers).
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
 * All other characters from fmt except the formatting directives are printed in
 * verbatim.
 *
 * @param fmt Formatting NULL terminated string (UTF-8 or plain ASCII).
 *
 * @return Number of UTF-8 characters printed, negative value on failure.
 *
 */
int printf_core(const char *fmt, printf_spec_t *ps, va_list ap)
{
	index_t i = 0;  /* Index of the currently processed character from fmt */
	index_t j = 0;  /* Index to the first not printed nonformating character */
	
	wchar_t uc;           /* Current UTF-32 character decoded from fmt */
	count_t counter = 0;  /* Number of UTF-8 characters printed */
	int retval;           /* Return values from nested functions */
	
	while ((uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT)) != 0) {
		/* Control character */
		if (uc == '%') {
			/* Print common characters if any processed */
			if (i > j) {
				if ((retval = printf_putnchars_utf8(&fmt[j], i - j, ps)) < 0) {
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
				i++;
				uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
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
				};
			} while (!end);
			
			/* Width & '*' operator */
			int width = 0;
			if (isdigit(uc)) {
				while ((uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT)) != 0) {
					if (!isdigit(uc))
						break;
					
					width *= 10;
					width += uc - '0';
					i++;
				}
			} else if (uc == '*') {
				/* Get width value from argument list */
				i++;
				uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
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
				i++;
				uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
				if (isdigit(uc)) {
					while ((uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT)) != 0) {
						if (!isdigit(uc))
							break;
						
						precision *= 10;
						precision += uc - '0';
						i++;
					}
				} else if (fmt[i] == '*') {
					/* Get precision value from the argument list */
					i++;
					uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
					precision = (int) va_arg(ap, int);
					if (precision < 0) {
						/* Ignore negative precision */
						precision = 0;
					}
				}
			}
			
			qualifier_t qualifier;
			
			switch (uc) {
			/** @todo Unimplemented qualifiers:
			 *        t ptrdiff_t - ISO C 99
			 */
			case 'h':
				/* Char or short */
				qualifier = PrintfQualifierShort;
				i++;
				uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
				if (uc == 'h') {
					i++;
					uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
					qualifier = PrintfQualifierByte;
				}
				break;
			case 'l':
				/* Long or long long */
				qualifier = PrintfQualifierLong;
				i++;
				uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
				if (uc == 'l') {
					i++;
					uc = utf8_decode(fmt, &i, UTF8_NO_LIMIT);
					qualifier = PrintfQualifierLongLong;
				}
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
					retval = print_utf32(va_arg(ap, wchar_t *), width, precision, flags, ps);
				else
					retval = print_utf8(va_arg(ap, char *), width, precision, flags, ps);
				
				if (retval < 0) {
					counter = -counter;
					goto out;
				}
				
				counter += retval;
				j = i + 1;
				goto next_char;
			case 'c':
				if (qualifier == PrintfQualifierLong)
					retval = print_wchar(va_arg(ap, wchar_t), width, flags, ps);
				else
					retval = print_char(va_arg(ap, unsigned int), width, flags, ps);
				
				if (retval < 0) {
					counter = -counter;
					goto out;
				};
				
				counter += retval;
				j = i + 1;
				goto next_char;
			
			/*
			 * Integer values
			 */
			case 'P':
				/* Pointer */
				flags |= __PRINTF_FLAG_BIGCHARS;
			case 'p':
				flags |= __PRINTF_FLAG_PREFIX;
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
			case 'u':
				break;
			case 'X':
				flags |= __PRINTF_FLAG_BIGCHARS;
			case 'x':
				base = 16;
				break;
			
			/* Percentile itself */
			case '%':
				j = i;
				goto next_char;
			
			/*
			 * Bad formatting.
			 */
			default:
				/*
				 * Unknown format. Now, j is the index of '%'
				 * so we will print whole bad format sequence.
				 */
				goto next_char;
			}
			
			/* Print integers */
			size_t size;
			uint64_t number;
			switch (qualifier) {
			case PrintfQualifierByte:
				size = sizeof(unsigned char);
				number = (uint64_t) va_arg(ap, unsigned int);
				break;
			case PrintfQualifierShort:
				size = sizeof(unsigned short);
				number = (uint64_t) va_arg(ap, unsigned int);
				break;
			case PrintfQualifierInt:
				size = sizeof(unsigned int);
				number = (uint64_t) va_arg(ap, unsigned int);
				break;
			case PrintfQualifierLong:
				size = sizeof(unsigned long);
				number = (uint64_t) va_arg(ap, unsigned long);
				break;
			case PrintfQualifierLongLong:
				size = sizeof(unsigned long long);
				number = (uint64_t) va_arg(ap, unsigned long long);
				break;
			case PrintfQualifierPointer:
				size = sizeof(void *);
				number = (uint64_t) (unsigned long) va_arg(ap, void *);
				break;
			default:
				/* Unknown qualifier */
				counter = -counter;
				goto out;
			}
			
			if (flags & __PRINTF_FLAG_SIGNED) {
				if (number & (0x1 << (size * 8 - 1))) {
					flags |= __PRINTF_FLAG_NEGATIVE;
					
					if (size == sizeof(uint64_t)) {
						number = -((int64_t) number);
					} else {
						number = ~number;
						number &=
						    ~(0xFFFFFFFFFFFFFFFFll <<
						    (size * 8));
						number++;
					}
				}
			}
			
			if ((retval = print_number(number, width, precision,
			    base, flags, ps)) < 0) {
				counter = -counter;
				goto out;
			}
			
			counter += retval;
			j = i + 1;
		}
next_char:
		
		i++;
	}
	
	if (i > j) {
		if ((retval = printf_putnchars_utf8(&fmt[j], i - j, ps)) < 0) {
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
