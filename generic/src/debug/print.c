/*
 * Copyright (C) 2001-2004 Jakub Jermar
 * Copyright (C) 2006 Josef Cejka
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

#include <putchar.h>
#include <print.h>
#include <synch/spinlock.h>
#include <arch/arg.h>
#include <arch/asm.h>

#include <arch.h>

SPINLOCK_INITIALIZE(printflock);		/**< printf spinlock */

#define __PRINTF_FLAG_PREFIX		0x00000001	/* show prefixes 0x or 0*/
#define __PRINTF_FLAG_SIGNED		0x00000002	/* signed / unsigned number */
#define __PRINTF_FLAG_ZEROPADDED	0x00000004	/* print leading zeroes */
#define __PRINTF_FLAG_LEFTALIGNED	0x00000010	/* align to left */
#define __PRINTF_FLAG_SHOWPLUS		0x00000020	/* always show + sign */
#define __PRINTF_FLAG_SPACESIGN		0x00000040	/* print space instead of plus */
#define __PRINTF_FLAG_BIGCHARS		0x00000080	/* show big characters */
#define __PRINTF_FLAG_NEGATIVE		0x00000100	/* number has - sign */

#define PRINT_NUMBER_BUFFER_SIZE	(64+5)		/* Buffer big enought for 64 bit number
							 * printed in base 2, sign, prefix and
							 * 0 to terminate string.. (last one is only for better testing 
							 * end of buffer by zero-filling subroutine)
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

static char digits_small[] = "0123456789abcdef"; 	/* Small hexadecimal characters */
static char digits_big[] = "0123456789ABCDEF"; 	/* Big hexadecimal characters */

static inline int isdigit(int c)
{
	return ((c >= '0' )&&( c <= '9'));
}

static __native strlen(const char *str) 
{
	__native counter = 0;

	while (str[counter] != 0) {
		counter++;
	}

	return counter;
}

/** Print one string without adding \n at end
 * Dont use this function directly - printflock is not locked here
 * */
static int putstr(const char *str)
{
	int count;
	if (str == NULL) {
		str = "(NULL)";
	}
	
	for (count = 0; str[count] != 0; count++) {
		putchar(str[count]);
	}
	return count;
}

/** Print count characters from buffer to output
 *
 */
static int putnchars(const char *buffer, __native count)
{
	int i;
	if (buffer == NULL) {
		buffer = "(NULL)";
		count = 6;
	}

	for (i = 0; i < count; i++) {
		putchar(buffer[i]);
	}
	
	return count;
}

/** Print one formatted character
 * @param c character to print
 * @param width 
 * @param flags
 * @return number of printed characters or EOF
 */
static int print_char(char c, int width, __u64 flags)
{
	int counter = 0;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (--width > 0) { 	/* one space is consumed by character itself hence predecrement */
			/* FIXME: painful slow */
			putchar(' ');	
			++counter;
		}
	}
	
 	putchar(c);
	++counter;

	while (--width > 0) { /* one space is consumed by character itself hence predecrement */
		putchar(' ');
		++counter;
	}
	
	return counter;
}

/** Print one string
 * @param s string
 * @param width 
 * @param precision
 * @param flags
 * @return number of printed characters or EOF
 */
						
static int print_string(char *s, int width, int precision, __u64 flags)
{
	int counter = 0;
	__native size;

	if (s == NULL) {
		return putstr("(NULL)");
	}
	
	size = strlen(s);

	/* print leading spaces */

	if (precision == 0) 
		precision = size;

	width -= precision;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) { 	
			putchar(' ');	
			counter++;
		}
	}

	while (precision > size) {
		precision--;
		putchar(' ');	
		++counter;
	}
	
 	if (putnchars(s, precision) == EOF) {
		return EOF;
	}

	counter += precision;

	while (width-- > 0) {
		putchar(' ');	
		++counter;
	}
	
	return ++counter;
}


/** Print number in given base
 *
 * Print significant digits of a number in given
 * base.
 *
 * @param num  Number to print.
 * @param width
 * @param precision
 * @param base Base to print the number in (should
 *             be in range 2 .. 16).
 * @param flags output modifiers
 * @return number of written characters or EOF
 *
 */
static int print_number(__u64 num, int width, int precision, int base , __u64 flags)
{
	char *digits = digits_small;
	char d[PRINT_NUMBER_BUFFER_SIZE];	/* this is good enough even for base == 2, prefix and sign */
	char *ptr = &d[PRINT_NUMBER_BUFFER_SIZE - 1];
	int size = 0;
	int written = 0;
	char sgn;
	
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

	/* Collect sum of all prefixes/signs/... to calculate padding and leading zeroes */
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

	/* if number is leftaligned or precision is specified then zeropadding is ignored */
	if (flags & __PRINTF_FLAG_ZEROPADDED) {
		if ((precision == 0) && (width > size)) {
			precision = width - size;
		}
	}

	/* print leading spaces */
	if (size > precision) /* We must print whole number not only a part */
		precision = size;

	width -= precision;
	
	if (!(flags & __PRINTF_FLAG_LEFTALIGNED)) {
		while (width-- > 0) { 	
			putchar(' ');	
			written++;
		}
	}
	
	/* print sign */
	if (sgn) {
		putchar(sgn);
		written++;
	}
	
	/* print prefix */
	
	if (flags & __PRINTF_FLAG_PREFIX) {
		switch(base) {
			case 2:	/* Binary formating is not standard, but usefull */
				putchar('0');
				if (flags & __PRINTF_FLAG_BIGCHARS) {
					putchar('B');
				} else {
					putchar('b');
				}
				written += 2;
				break;
			case 8:
				putchar('o');
				written++;
				break;
			case 16:
				putchar('0');
				if (flags & __PRINTF_FLAG_BIGCHARS) {
					putchar('X');
				} else {
					putchar('x');
				}
				written += 2;
				break;
		}
	}

	/* print leading zeroes */
	precision -= size;
	while (precision-- > 0) { 	
		putchar('0');	
		written++;
	}

	
	/* print number itself */

	written += putstr(++ptr);
	
	/* print ending spaces */
	
	while (width-- > 0) { 	
		putchar(' ');	
		written++;
	}

	return written;
}



/** General formatted text print
 *
 * Print text formatted according the fmt parameter
 * and variant arguments. Each formatting directive
 * begins with \% (percentage) character and one of the
 * following character:
 *
 * \%    Prints the percentage character.
 *
 * s    The next variant argument is treated as char*
 *      and printed as a NULL terminated string.
 *
 * c    The next variant argument is treated as a single char.
 *
 * p    The next variant argument is treated as a maximum
 *      bit-width integer with respect to architecture
 *      and printed in full hexadecimal width.
 *
 * P    As with 'p', but '0x' is prefixed.
 *
 * q    The next variant argument is treated as a 64b integer
 *      and printed in full hexadecimal width.
 *
 * Q    As with 'q', but '0x' is prefixed.
 *
 * l    The next variant argument is treated as a 32b integer
 *      and printed in full hexadecimal width.
 *
 * L    As with 'l', but '0x' is prefixed.
 *
 * w    The next variant argument is treated as a 16b integer
 *      and printed in full hexadecimal width.
 *
 * W    As with 'w', but '0x' is prefixed.
 *
 * b    The next variant argument is treated as a 8b integer
 *      and printed in full hexadecimal width.
 *
 * B    As with 'b', but '0x' is prefixed.
 *
 * d    The next variant argument is treated as integer
 *      and printed in standard decimal format (only significant
 *      digits).
 *
 * x    The next variant argument is treated as integer
 *      and printed in standard hexadecimal format (only significant
 *      digits).
 *
 * X    As with 'x', but '0x' is prefixed.
 *
 * All other characters from fmt except the formatting directives
 * are printed in verbatim.
 *
 * @param fmt Formatting NULL terminated string.
 */
int printf(const char *fmt, ...)
{
	int irqpri;
	int i = 0, j = 0; /* i is index of currently processed char from fmt, j is index to the first not printed nonformating character */
	int end;
	int counter; /* counter of printed characters */
	int retval; /* used to store return values from called functions */
	va_list ap;
	char c;
	qualifier_t qualifier;	/* type of argument */
	int base;	/* base in which will be parameter (numbers only) printed */
	__u64 number; /* argument value */
	__native size; /* byte size of integer parameter */
	int width, precision;
	__u64 flags;
	
	counter = 0;
	
	va_start(ap, fmt);
	
	irqpri = interrupts_disable();
	spinlock_lock(&printflock);

	
	while ((c = fmt[i])) {
		/* control character */
		if (c == '%' ) { 
			/* print common characters if any processed */	
			if (i > j) {
				if ((retval = putnchars(&fmt[j], (__native)(i - j))) == EOF) { /* error */
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
					case '#': flags |= __PRINTF_FLAG_PREFIX; break;
					case '-': flags |= __PRINTF_FLAG_LEFTALIGNED; break;
					case '+': flags |= __PRINTF_FLAG_SHOWPLUS; break;
					case ' ': flags |= __PRINTF_FLAG_SPACESIGN; break;
					case '0': flags |= __PRINTF_FLAG_ZEROPADDED; break;
					default: end = 1;
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
				/* get width value from argument list*/
				i++;
				width = (int)va_arg(ap, int);
				if (width < 0) {
					/* negative width means to set '-' flag */
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
					/* get precision value from argument list*/
					i++;
					precision = (int)va_arg(ap, int);
					if (precision < 0) {
						/* negative precision means to ignore it */
						precision = 0;
					}
				}
			}

			switch (fmt[i++]) {
				/** TODO: unimplemented qualifiers:
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
				case 'z':	/* __native */
					qualifier = PrintfQualifierNative;
					break;
				default:
					qualifier = PrintfQualifierInt; /* default type */
					--i;
			}	
			
			base = 10;

			switch (c = fmt[i]) {

				/*
				* String and character conversions.
				*/
				case 's':
					if ((retval = print_string(va_arg(ap, char*), width, precision, flags)) == EOF) {
						counter = -counter;
						goto out;
					};
					
					counter += retval;
					j = i + 1; 
					goto next_char;
				case 'c':
					c = va_arg(ap, unsigned int);
					if ((retval = print_char(c, width, flags )) == EOF) {
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
					/* Unknown format
					 *  now, the j is index of '%' so we will
					 * print whole bad format sequence
					 */
					goto next_char;		
			}
		
		
		/* Print integers */
			/* print number */
			switch (qualifier) {
				case PrintfQualifierByte:
					size = sizeof(unsigned char);
					number = (__u64)va_arg(ap, unsigned int);
					break;
				case PrintfQualifierShort:
					size = sizeof(unsigned short);
					number = (__u64)va_arg(ap, unsigned int);
					break;
				case PrintfQualifierInt:
					size = sizeof(unsigned int);
					number = (__u64)va_arg(ap, unsigned int);
					break;
				case PrintfQualifierLong:
					size = sizeof(unsigned long);
					number = (__u64)va_arg(ap, unsigned long);
					break;
				case PrintfQualifierLongLong:
					size = sizeof(unsigned long long);
					number = (__u64)va_arg(ap, unsigned long long);
					break;
				case PrintfQualifierPointer:
					size = sizeof(void *);
					number = (__u64)(unsigned long)va_arg(ap, void *);
					break;
				case PrintfQualifierNative:
					size = sizeof(__native);
					number = (__u64)va_arg(ap, __native);
					break;
				default: /* Unknown qualifier */
					counter = -counter;
					goto out;
					
			}
			
			if (flags & __PRINTF_FLAG_SIGNED) {
				if (number & (0x1 << (size*8 - 1))) {
					flags |= __PRINTF_FLAG_NEGATIVE;
				
					if (size == sizeof(__u64)) {
						number = -((__s64)number);
					} else {
						number = ~number;
						number &= (~((0xFFFFFFFFFFFFFFFFll) <<  (size * 8)));
						number++;
					}
				}
			}

			if ((retval = print_number(number, width, precision, base, flags)) == EOF ) {
				counter = -counter;
				goto out;
			};

			counter += retval;
			j = i + 1;
		}	
next_char:
			
		++i;
	}
	
	if (i > j) {
		if ((retval = putnchars(&fmt[j], (__native)(i - j))) == EOF) { /* error */
			counter = -counter;
			goto out;
		}
		counter += retval;
	}
out:
	spinlock_unlock(&printflock);
	interrupts_restore(irqpri);
	
	va_end(ap);
	return counter;
}

