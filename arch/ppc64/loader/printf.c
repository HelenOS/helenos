/*
 * Copyright (C) 2006 Martin Decky
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

#include "printf.h"
#include "ofw.h"

static char digits[] = "0123456789abcdef";  /**< Hexadecimal characters */


void puts(const char *str)
{
	long len = 0;
	
	while (str[len] != 0)
		len++;
	
	ofw_write(str, len);
}


/** Print hexadecimal digits
 *
 * Print fixed count of hexadecimal digits from
 * the number num. The digits are printed in
 * natural left-to-right order starting with
 * the width-th digit.
 *
 * @param num   Number containing digits.
 * @param width Count of digits to print.
 *
 */
static void print_fixed_hex(const __u64 num, const int width)
{
	int i;
    
	for (i = width * 8 - 4; i >= 0; i -= 4)
	    ofw_write(digits + ((num >> i) & 0xf), 1);
}


/** Print number in given base
 *
 * Print significant digits of a number in given
 * base.
 *
 * @param num  Number to print.
 * @param base Base to print the number in (should
 *             be in range 2 .. 16).
 *
 */
static void print_number(const __native num, const unsigned int base)
{
	int val = num;
	char d[sizeof(__native) * 8 + 1];		/* this is good enough even for base == 2 */
	int i = sizeof(__native) * 8 - 1;
	
	do {
		d[i--] = digits[val % base];
	} while (val /= base);
	
	d[sizeof(__native) * 8] = 0;	
	puts(&d[i + 1]);
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
void printf(const char *fmt, ...)
{
	long i = 0;
	va_list ap;
	char c;	
	
	va_start(ap, fmt);
	
	while ((c = fmt[i++])) {
		switch (c) {
			
			/* control character */
			case '%':
				
				switch (c = fmt[i++]) {
					
					/* percentile itself */
					case '%':
						break;
					
					/*
					 * String and character conversions.
					 */
					case 's':
						puts(va_arg(ap, char_ptr));
						goto loop;
					
					case 'c':
						c = (char) va_arg(ap, int);
						break;
					
					/*
					 * Hexadecimal conversions with fixed width.
					 */
					case 'P': 
						puts("0x");
					case 'p':
						print_fixed_hex(va_arg(ap, __native), sizeof(__native));
						goto loop;
					
					case 'Q':
						puts("0x");
					case 'q':
						print_fixed_hex(va_arg(ap, __u64), INT64);
						goto loop;
					
					case 'L': 
						puts("0x");
					case 'l':
						print_fixed_hex(va_arg(ap, __native), INT32);
						goto loop;
					
					case 'W':
						puts("0x");
					case 'w':
						print_fixed_hex(va_arg(ap, __native), INT16);
						goto loop;
					
					case 'B':
						puts("0x");
					case 'b':
						print_fixed_hex(va_arg(ap, __native), INT8);
						goto loop;
					
					/*
					 * Decimal and hexadecimal conversions.
					 */
					case 'd':
						print_number(va_arg(ap, __native), 10);
						goto loop;
					
					case 'X':
						puts("0x");
					case 'x':
						print_number(va_arg(ap, __native), 16);
						goto loop;
					
					/*
					 * Bad formatting.
					 */
					default:
						goto out;
			}
			
			default:
				if (c == '\n')
					ofw_write("\r", 1);
				ofw_write(&c, 1);
		}
	
loop:
		;
	}
	
out:
	
	va_end(ap);
}
