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

#include <stdio.h>
#include <unistd.h>
#include <io/io.h>
#include <stdarg.h>

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
	PrintfQualifierSizeT,
	PrintfQualifierPointer
} qualifier_t;

static char digits_small[] = "0123456789abcdef"; 	/* Small hexadecimal characters */
static char digits_big[] = "0123456789ABCDEF"; 	/* Big hexadecimal characters */

/** Print number in given base
 *
 * Print significant digits of a number in given
 * base.
 *
 * @param num  Number to print.
 * @param size not used, in future releases will be replaced with precision and width params
 * @param base Base to print the number in (should
 *             be in range 2 .. 16).
 * @param flags output modifiers
 * @return number of written characters or EOF
 *
 */
static int print_number(uint64_t num, size_t size, int base , uint64_t flags)
{
	/* FIXME: This is only first version.
	 * Printf does not have support for specification of size 
	 * and precision, so this function writes with parameters defined by
	 * their type size.
	 */
	char *digits = digits_small;
	char d[PRINT_NUMBER_BUFFER_SIZE];	/* this is good enough even for base == 2, prefix and sign */
	char *ptr = &d[PRINT_NUMBER_BUFFER_SIZE - 1];
	
	if (flags & __PRINTF_FLAG_BIGCHARS) 
		digits = digits_big;	
	
	*ptr-- = 0; /* Put zero at end of string */

	if (num == 0) {
		*ptr-- = '0';
	} else {
		do {
			*ptr-- = digits[num % base];
		} while (num /= base);
	}
	if (flags & __PRINTF_FLAG_PREFIX) { /*FIXME*/
		switch(base) {
			case 2:	/* Binary formating is not standard, but usefull */
				*ptr = 'b';
				if (flags & __PRINTF_FLAG_BIGCHARS) *ptr = 'B';
				ptr--;
				*ptr-- = '0';
				break;
			case 8:
				*ptr-- = 'o';
				break;
			case 16:
				*ptr = 'x';
				if (flags & __PRINTF_FLAG_BIGCHARS) *ptr = 'X';
				ptr--;
				*ptr-- = '0';
				break;
		}
	}
	
	if (flags & __PRINTF_FLAG_SIGNED) {
		if (flags & __PRINTF_FLAG_NEGATIVE) {
			*ptr-- = '-';
		} else if (flags & __PRINTF_FLAG_SHOWPLUS) {
				*ptr-- = '+';
			} else if (flags & __PRINTF_FLAG_SPACESIGN) {
					*ptr-- = ' ';
				}
	}

	/* Print leading zeroes */

	if (flags & __PRINTF_FLAG_LEFTALIGNED) {
		flags &= ~__PRINTF_FLAG_ZEROPADDED;
	}
	if (flags & __PRINTF_FLAG_ZEROPADDED) {
	       	while (ptr != d ) {
			*ptr-- = '0';
		}
	}
	
	return putstr(++ptr);
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
	int i = 0, j = 0; /* i is index of currently processed char from fmt, j is index to the first not printed nonformating character */
	int end;
	int counter; /* counter of printed characters */
	int retval; /* used to store return values from called functions */
	va_list ap;
	char c;
	qualifier_t qualifier;	/* type of argument */
	int base;	/* base in which will be parameter (numbers only) printed */
	uint64_t number; /* argument value */
	size_t	size; /* byte size of integer parameter */
	uint64_t flags;
	
	counter = 0;
	va_start(ap, fmt);

	while ((c = fmt[i])) {
		/* control character */
		if (c == '%' ) { 
			/* print common characters if any processed */	
			if (i > j) {
				if ((retval = putnchars(&fmt[j], (size_t)(i - j))) == EOF) { /* error */
					return -counter;
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
			/* TODO: width & '*' operator */
			/* TODO: precision and '*' operator */	

			switch (fmt[i++]) {
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
				case 'z':	/* size_t */
					qualifier = PrintfQualifierSizeT;
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
					if ((retval = putstr(va_arg(ap, char*))) == EOF) {
						return -counter;
					};
					
					counter += retval;
					j = i + 1; 
					goto next_char;
				case 'c':
					c = va_arg(ap, unsigned long);
					if ((retval = putnchars(&c, sizeof(char))) == EOF) {
						return -counter;
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
					number = (uint64_t)va_arg(ap, unsigned long long);
					break;
				case PrintfQualifierPointer:
					size = sizeof(void *);
					number = (uint64_t)(unsigned long)va_arg(ap, void *);
					break;
				case PrintfQualifierSizeT:
					size = sizeof(size_t);
					number = (uint64_t)va_arg(ap, size_t);
					break;
				default: /* Unknown qualifier */
					return -counter;
					
			}
			
			if (flags & __PRINTF_FLAG_SIGNED) {
				if (number & (0x1 << (size*8 - 1))) {
					flags |= __PRINTF_FLAG_NEGATIVE;
				
					if (size == sizeof(uint64_t)) {
						number = -((int64_t)number);
					} else {
						number = ~number;
						number &= (~((0xFFFFFFFFFFFFFFFFll) <<  (size * 8)));
						number++;
					}
				}
			}

			if ((retval = print_number(number, size, base, flags)) == EOF ) {
				return -counter;
			};

			counter += retval;
			j = i + 1;
		}	
next_char:
			
		++i;
	}
	
	if (i > j) {
		if ((retval = putnchars(&fmt[j], (size_t)(i - j))) == EOF) { /* error */
			return -counter;
		}
		counter += retval;
	}
	
	va_end(ap);
	return counter;
}

