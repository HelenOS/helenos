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

static char digits[] = "0123456789abcdef"; 	/**< Hexadecimal characters */

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
static int print_fixed_hex(const uint64_t num, const int width)
{
	int i;
	char buf[16];
	char *bptr;
	
	bptr = buf;
	for (i = width*8 - 4; i >= 0; i -= 4)
		*bptr++ = digits[(num>>i) & 0xf];
	*bptr = '\0';
	
	return putstr(buf);	
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
static int print_number(const unsigned int num, const unsigned int base)
{
	int val = num;
	char d[sizeof(unsigned int)*8+1];	/* this is good enough even for base == 2 */
	int i = sizeof(unsigned int)*8-1;
	
	do {
		d[i--] = digits[val % base];
	} while (val /= base);
	
	d[sizeof(unsigned int)*8] = 0;	

	return putstr(&d[i + 1]);
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
	int i = 0, j = 0;
	int counter, retval;
	va_list ap;
	char c;	
	
	counter = 0;
	va_start(ap, fmt);

	while ((c = fmt[i])) {
			/* control character */
		if (c == '%' ) { 
				/* print common characters if any processed */	
			if (i > j) {
				if ((retval = putnchars(&fmt[j], i - j)) == EOF) { /* error */
					return -counter;
				}
				counter += retval;
			}

			j = ++i;

			switch (c = fmt[i]) {

				/* percentile itself */
				case '%': 
					--j;	/* soon will be incremented back */
					break;

				/*
				* String and character conversions.
				*/
				case 's':
					if ((retval = putstr(va_arg(ap, char*))) == EOF) {
						return -counter;
					};
					
					counter += retval;
					break;
				case 'c':
					if ((retval = putnchars((char *)&va_arg(ap, int), sizeof(char))) == EOF) {
						return -counter;
					};
					
					counter += retval;
					break;

				/*
				* Hexadecimal conversions with fixed width.
				*/
				case 'P': 
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'p':
	    				if ((retval = print_fixed_hex(va_arg(ap, int), sizeof(int))) == EOF ) {
						return -counter;
					};

					counter += retval;
					break;
				case 'Q':
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'q':
	    				if ((retval = print_fixed_hex(va_arg(ap, uint64_t), sizeof(uint64_t))) == EOF ) {
						return -counter;
					};

					counter += retval;
					break;
				case 'L': 
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'l':
	    				if ((retval = print_fixed_hex(va_arg(ap, int), sizeof(uint32_t))) == EOF ) {
						return -counter;
					};

					counter += retval;
			   		break; 
				case 'W':
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'w':
	    				if ((retval = print_fixed_hex(va_arg(ap, int), sizeof(uint16_t))) == EOF ) {
						return -counter;
					};

					counter += retval;
					break;
				case 'B':
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'b':
	    				if ((retval = print_fixed_hex(va_arg(ap, int), sizeof(uint8_t))) == EOF ) {
						return -counter;
					};

					counter += retval;
					break;
				/*
				* Decimal and hexadecimal conversions.
				*/
				case 'd':
	    				if ((retval = print_number(va_arg(ap, int), 10)) == EOF ) {
						return -counter;
					};

					counter += retval;
			   		break; 
				case 'X':
					if ((retval = putnchars("0x", 2)) == EOF) {
						return -counter;
					};
					
					counter += retval;
				case 'x':
	    				if ((retval = print_number(va_arg(ap, int), 16)) == EOF ) {
						return -counter;
					};

					counter += retval;
			   		break; 
				/*
				* Bad formatting.
				*/
				default:
					return -counter;
			}
			++j;
		}	
		++i;
	}
	
	va_end(ap);
	return counter;
}

