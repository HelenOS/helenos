/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
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
 * @brief	Printing functions.
 */

#include <printf/printf_core.h>
#include <putchar.h>
#include <print.h>
#include <arch/arg.h>
#include <macros.h>
#include <func.h>
#include <arch.h>

/** show prefixes 0x or 0 */
#define __PRINTF_FLAG_PREFIX		0x00000001
/** signed / unsigned number */
#define __PRINTF_FLAG_SIGNED		0x00000002
/** print leading zeroes */
#define __PRINTF_FLAG_ZEROPADDED	0x00000004
/** align to left */
#define __PRINTF_FLAG_LEFTALIGNED	0x00000010
/** always show + sign */
#define __PRINTF_FLAG_SHOWPLUS		0x00000020
/** print space instead of plus */
#define __PRINTF_FLAG_SPACESIGN		0x00000040
/** show big characters */
#define __PRINTF_FLAG_BIGCHARS		0x00000080
/** number has - sign */
#define __PRINTF_FLAG_NEGATIVE		0x00000100

/**
 * Buffer big enough for 64-bit number printed in base 2, sign, prefix and 0
 * to terminate string... (last one is only for better testing end of buffer by
 * zero-filling subroutine)
 */
#define PRINT_NUMBER_BUFFER_SIZE	(64 + 5)	

/** Enumeration of possible arguments types.
 */
typedef enum {
	PrintfQualifierByte = 0,
	PrintfQualifierShort,
	PrintfQualifierInt,
	PrintfQualifierLong,
	PrintfQualifierLongLong,
	PrintfQualifierNative,
	PrintfQualifierPointer
} qualifier_t;

static char digits_small[] = "0123456789abcdef";
static char digits_big[] = "0123456789ABCDEF";

/** Print one or more characters without adding newline.
 *
 * @param buf		Buffer with size at least count bytes. NULL pointer is
 *			not allowed!
 * @param count		Number of characters to print.
 * @param ps		Output method and its data.
 * @return		Number of characters printed.
 */
static int printf_putnchars(const char * buf, size_t count,
    struct printf_spec *ps)
{
	return ps->write((void *)buf, count, ps->data);
}

/** Print a string without adding a newline.
 *
 * @param str		String to print.
 * @param ps		Write function specification and support data.
 * @return		Number of characters printed.
 */
static int printf_putstr(const char * str, struct printf_spec *ps)
{
	size_t count;
	
	if (str == NULL) {
		char *nullstr = "(NULL)";
		return printf_putnchars(nullstr, strlen(nullstr), ps);
	}

	count = strlen(str);

	return ps->write((void *) str, count, ps->data);
}

/** Print one character.
 *
 * @param c		Character to be printed.
 * @param ps		Output method.
 *
 * @return		Number of characters printed.
 */
static int printf_putchar(int c, struct printf_spec *ps)
{
	unsigned char ch = c;
	
	return ps->write((void *) &ch, 1, ps->data);
}

/** Print one formatted character.
 *
 * @param c		Character to print.
 * @param width		Width modifier.
 * @param flags		Flags that change the way the character is printed.
 *
 * @return		Number of characters printed, negative value on failure.
 */
static int print_char(char c, int width, uint64_t flags, struct printf_spec *ps)
{
	int counter = 0;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (--width > 0) {
			/*
			 * One space is consumed by the character itself, hence
			 * the predecrement.
			 */
			if (printf_putchar(' ', ps) > 0)	
				++counter;
		}
	}
	
 	if (printf_putchar(c, ps) > 0)
		counter++;

	while (--width > 0) {
			/*
			 * One space is consumed by the character itself, hence
			 * the predecrement.
			 */
		if (printf_putchar(' ', ps) > 0)
			++counter;
	}
	
	return ++counter;
}

/** Print string.
 *
 * @param s		String to be printed.
 * @param width		Width modifier.
 * @param precision	Precision modifier.
 * @param flags		Flags that modify the way the string is printed.
 *
 * @return		Number of characters printed, negative value on	failure.
 */ 
static int print_string(char *s, int width, int precision, uint64_t flags,
    struct printf_spec *ps)
{
	int counter = 0;
	size_t size;
	int retval;

	if (s == NULL) {
		return printf_putstr("(NULL)", ps);
	}
	
	size = strlen(s);

	/* print leading spaces */

	if (precision == 0) 
		precision = size;

	width -= precision;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) { 	
			if (printf_putchar(' ', ps) == 1)	
				counter++;
		}
	}

 	if ((retval = printf_putnchars(s, min(size, precision), ps)) < 0) {
		return -counter;
	}
	counter += retval;	

	while (width-- > 0) {
		if (printf_putchar(' ', ps) == 1)	
			++counter;
	}
	
	return counter;
}


/** Print a number in a given base.
 *
 * Print significant digits of a number in given base.
 *
 * @param num		Number to print.
 * @param widt		Width modifier.h
 * @param precision	Precision modifier.
 * @param base		Base to print the number in (must be between 2 and 16).
 * @param flags		Flags that modify the way the number is printed.	
 *
 * @return		Number of characters printed.
 *
 */
static int print_number(uint64_t num, int width, int precision, int base,
    uint64_t flags, struct printf_spec *ps)
{
	char *digits = digits_small;
	char d[PRINT_NUMBER_BUFFER_SIZE];
	char *ptr = &d[PRINT_NUMBER_BUFFER_SIZE - 1];
	int size = 0;		/* size of number with all prefixes and signs */
	int number_size; 	/* size of plain number */
	char sgn;
	int retval;
	int counter = 0;
	
	if (flags & __PRINTF_FLAG_BIGCHARS) 
		digits = digits_big;	
	
	*ptr-- = 0; /* Put zero at end of string */

	if (num == 0) {
		*ptr-- = '0';
		size++;
	} else {
		do {
			*ptr-- = digits[num % base];
			size++;
		} while (num /= base);
	}
	
	number_size = size;

	/*
	 * Collect the sum of all prefixes/signs/... to calculate padding and
	 * leading zeroes.
	 */
	if (flags & __PRINTF_FLAG_PREFIX) {
		switch(base) {
		case 2:	/* Binary formating is not standard, but usefull */
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

	sgn = 0;
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

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		flags &= ~__PRINTF_FLAG_ZEROPADDED;
	}

	/*
	 * If the number is leftaligned or precision is specified then
	 * zeropadding is ignored.
	 */
	if (flags & __PRINTF_FLAG_ZEROPADDED) {
		if ((precision == 0) && (width > size)) {
			precision = width - size + number_size;
		}
	}

	/* print leading spaces */
	if (number_size > precision) {
		/* print the whole number not only a part */
		precision = number_size;
	}

	width -= precision + size - number_size;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) { 	
			if (printf_putchar(' ', ps) == 1)	
				counter++;
		}
	}
	
	
	/* print sign */
	if (sgn) {
		if (printf_putchar(sgn, ps) == 1)
			counter++;
	}
	
	/* print prefix */
	
	if (flags & __PRINTF_FLAG_PREFIX) {
		switch(base) {
		case 2:	/* Binary formating is not standard, but usefull */
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

	/* print leading zeroes */
	precision -= number_size;
	while (precision-- > 0) { 	
		if (printf_putchar('0', ps) == 1)
			counter++;
	}

	
	/* print number itself */

	if ((retval = printf_putstr(++ptr, ps)) > 0) {
		counter += retval;
	}
	
	/* print ending spaces */
	
	while (width-- > 0) { 	
		if (printf_putchar(' ', ps) == 1)	
			counter++;
	}

	return counter;
}


/** Print formatted string.
 *
 * Print string formatted according to the fmt parameter and variadic arguments.
 * Each formatting directive must have the following form:
 * 
 * 	\% [ FLAGS ] [ WIDTH ] [ .PRECISION ] [ TYPE ] CONVERSION
 *
 * FLAGS:@n
 * 	- "#"	Force to print prefix.For \%o conversion, the prefix is 0, for
 *		\%x and \%X prefixes are 0x and	0X and for conversion \%b the
 *		prefix is 0b.
 *
 * 	- "-"	Align to left.
 *
 * 	- "+"	Print positive sign just as negative.
 *
 * 	- " "	If the printed number is positive and "+" flag is not set,
 *		print space in place of sign.
 *
 * 	- "0"	Print 0 as padding instead of spaces. Zeroes are placed between
 *		sign and the rest of the number. This flag is ignored if "-"
 *		flag is specified.
 * 
 * WIDTH:@n
 * 	- Specify the minimal width of a printed argument. If it is bigger,
 *	width is ignored. If width is specified with a "*" character instead of
 *	number, width is taken from parameter list. And integer parameter is
 *	expected before parameter for processed conversion specification. If
 *	this value is negative its absolute value is taken and the "-" flag is
 *	set.
 *
 * PRECISION:@n
 * 	- Value precision. For numbers it specifies minimum valid numbers.
 *	Smaller numbers are printed with leading zeroes. Bigger numbers are not
 *	affected. Strings with more than precision characters are cut off. Just
 *	as with width, an "*" can be used used instead of a number. An integer
 *	value is then expected in parameters. When both width and precision are
 *	specified using "*", the first parameter is used for width and the
 *	second one for precision.
 * 
 * TYPE:@n
 * 	- "hh"	Signed or unsigned char.@n
 * 	- "h"	Signed or unsigned short.@n
 * 	- ""	Signed or unsigned int (default value).@n
 * 	- "l"	Signed or unsigned long int.@n
 * 	- "ll"	Signed or unsigned long long int.@n
 * 	- "z"	unative_t (non-standard extension).@n
 * 
 * 
 * CONVERSION:@n
 * 	- %	Print percentile character itself.
 *
 * 	- c	Print single character.
 *
 * 	- s	Print zero terminated string. If a NULL value is passed as
 *		value, "(NULL)" is printed instead.
 * 
 * 	- P, p	Print value of a pointer. Void * value is expected and it is
 *		printed in hexadecimal notation with prefix (as with \%#X / \%#x
 *		for 32-bit or \%#X / \%#x for 64-bit long pointers).
 *
 * 	- b	Print value as unsigned binary number. Prefix is not printed by
 *		default. (Nonstandard extension.)
 * 
 * 	- o	Print value as unsigned octal number. Prefix is not printed by
 *		default. 
 *
 * 	- d, i	Print signed decimal number. There is no difference between d
 *		and i conversion.
 *
 * 	- u	Print unsigned decimal number.
 *
 * 	- X, x	Print hexadecimal number with upper- or lower-case. Prefix is
 *		not printed by default.
 * 
 * All other characters from fmt except the formatting directives are printed in
 * verbatim.
 *
 * @param fmt		Formatting NULL terminated string.
 * @return		Number of characters printed, negative value on failure.
 */
int printf_core(const char *fmt, struct printf_spec *ps, va_list ap)
{
	int i = 0; /* index of the currently processed char from fmt */
	int j = 0; /* index to the first not printed nonformating character */
	int end;
	int counter; /* counter of printed characters */
	int retval; /* used to store return values from called functions */
	char c;
	qualifier_t qualifier; /* type of argument */
	int base; /* base in which a numeric parameter will be printed */
	uint64_t number; /* argument value */
	size_t	size; /* byte size of integer parameter */
	int width, precision;
	uint64_t flags;
	
	counter = 0;
		
	while ((c = fmt[i])) {
		/* control character */
		if (c == '%' ) { 
			/* print common characters if any processed */	
			if (i > j) {
				if ((retval = printf_putnchars(&fmt[j],
				    (size_t)(i - j), ps)) < 0) { /* error */
					counter = -counter;
					goto out;
				}
				counter += retval;
			}
		
			j = i;
			/* parse modifiers */
			flags = 0;
			end = 0;
			
			do {
				++i;
				switch (c = fmt[i]) {
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
					end = 1;
				};	
				
			} while (end == 0);	
			
			/* width & '*' operator */
			width = 0;
			if (isdigit(fmt[i])) {
				while (isdigit(fmt[i])) {
					width *= 10;
					width += fmt[i++] - '0';
				}
			} else if (fmt[i] == '*') {
				/* get width value from argument list */
				i++;
				width = (int)va_arg(ap, int);
				if (width < 0) {
					/* negative width sets '-' flag */
					width *= -1;
					flags |= __PRINTF_FLAG_LEFTALIGNED;
				}
			}
			
			/* precision and '*' operator */	
			precision = 0;
			if (fmt[i] == '.') {
				++i;
				if (isdigit(fmt[i])) {
					while (isdigit(fmt[i])) {
						precision *= 10;
						precision += fmt[i++] - '0';
					}
				} else if (fmt[i] == '*') {
					/*
					 * Get precision value from the argument
					 * list.
					 */
					i++;
					precision = (int)va_arg(ap, int);
					if (precision < 0) {
						/* ignore negative precision */
						precision = 0;
					}
				}
			}

			switch (fmt[i++]) {
			/** @todo unimplemented qualifiers:
			 * t ptrdiff_t - ISO C 99
			 */
			case 'h':	/* char or short */
				qualifier = PrintfQualifierShort;
				if (fmt[i] == 'h') {
					i++;
					qualifier = PrintfQualifierByte;
				}
				break;
			case 'l':	/* long or long long*/
				qualifier = PrintfQualifierLong;
				if (fmt[i] == 'l') {
					i++;
					qualifier = PrintfQualifierLongLong;
				}
				break;
			case 'z':	/* unative_t */
				qualifier = PrintfQualifierNative;
				break;
			default:
				/* default type */
				qualifier = PrintfQualifierInt; 
				--i;
			}	
			
			base = 10;

			switch (c = fmt[i]) {

			/*
			* String and character conversions.
			*/
			case 's':
				if ((retval = print_string(va_arg(ap, char *),
				    width, precision, flags, ps)) < 0) {
					counter = -counter;
					goto out;
				};

				counter += retval;
				j = i + 1; 
				goto next_char;
			case 'c':
				c = va_arg(ap, unsigned int);
				retval = print_char(c, width, flags, ps);
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
			case 'P': /* pointer */
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
			/* percentile itself */
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
			/* print number */
			switch (qualifier) {
			case PrintfQualifierByte:
				size = sizeof(unsigned char);
				number = (uint64_t)va_arg(ap, unsigned int);
				break;
			case PrintfQualifierShort:
				size = sizeof(unsigned short);
				number = (uint64_t)va_arg(ap, unsigned int);
				break;
			case PrintfQualifierInt:
				size = sizeof(unsigned int);
				number = (uint64_t)va_arg(ap, unsigned int);
				break;
			case PrintfQualifierLong:
				size = sizeof(unsigned long);
				number = (uint64_t)va_arg(ap, unsigned long);
				break;
			case PrintfQualifierLongLong:
				size = sizeof(unsigned long long);
				number = (uint64_t)va_arg(ap,
				    unsigned long long);
				break;
			case PrintfQualifierPointer:
				size = sizeof(void *);
				number = (uint64_t)(unsigned long)va_arg(ap,
				    void *);
				break;
			case PrintfQualifierNative:
				size = sizeof(unative_t);
				number = (uint64_t)va_arg(ap, unative_t);
				break;
			default: /* Unknown qualifier */
				counter = -counter;
				goto out;
			}
			
			if (flags & __PRINTF_FLAG_SIGNED) {
				if (number & (0x1 << (size * 8 - 1))) {
					flags |= __PRINTF_FLAG_NEGATIVE;
				
					if (size == sizeof(uint64_t)) {
						number = -((int64_t)number);
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
			
		++i;
	}
	
	if (i > j) {
		if ((retval = printf_putnchars(&fmt[j], (unative_t)(i - j),
		    ps)) < 0) { /* error */
			counter = -counter;
			goto out;
			
		}
		counter += retval;
	}

out:
	
	return counter;
}

/** @}
 */
