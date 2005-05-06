/*
 * Copyright (C) 2001-2004 Jakub Jermar
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


static char digits[] = "0123456789abcdef";
static spinlock_t printflock;

void print_str(char *str)
{
        int i = 0;
	char c;
    
	while (c = str[i++])
	    putchar(c);
}


/* 
 * This is a universal function for printing hexadecimal numbers of fixed
 * width.
 */
void print_fixed_hex(__u32 num, int width)
{
	int i;
    
	for (i = width*8 - 4; i >= 0; i -= 4)
	    putchar(digits[(num>>i) & 0xf]);
}

/* 
 * This is a universal function for printing decimal and hexadecimal numbers.
 * It prints only significant digits.
 */
void print_number(__u32 num, int base)
{ 
	char d[32+1];		/* this is good enough even for base == 2 */
        int i = 31;
    
	do {
		d[i--] = digits[num % base];
	} while (num /= base);
	
	d[32] = 0;	
	print_str(&d[i + 1]);
}

/*
 * This is our function for printing formatted text.
 * It's much simpler than the user-space one.
 * We are greateful for this function.
 */
void printf(char *fmt, ...)
{
	int irqpri, i = 0;
	va_list ap;
	char c;	

	va_start(ap, fmt);

	irqpri = cpu_priority_high();
	spinlock_lock(&printflock);

	while (c = fmt[i++]) {
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
					print_str(va_arg(ap, char_ptr));
					goto loop;

				case 'c':
					c = va_arg(ap, char);
					break;

				/*
		                 * Hexadecimal conversions with fixed width.
		                 */
				case 'L': 
					print_str("0x");
				case 'l':
		    			print_fixed_hex(va_arg(ap, __native), INT32);
					goto loop;

				case 'W':
					print_str("0x");
				case 'w':
		    			print_fixed_hex(va_arg(ap, __native), INT16);
					goto loop;

				case 'B':
					print_str("0x");
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
			                print_str("0x");
				case 'x':
		    			print_number(va_arg(ap, __native), 16);
					goto loop;
	    
				/*
				 * Bad formatting.
				 */
				default:
					goto out;
			    }

		    default: putchar(c);
		}
	
loop:
		;
	}

out:
	spinlock_unlock(&printflock);
	cpu_priority_restore(irqpri);
	
	va_end(ap);
}
