/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Debug print functions.
 */

#include <printf/printf_core.h>
#include <arch/debug/print.h>
#include <arch/machine.h>

/** Prints a character to the console.
 *
 * @param ch Character to be printed.
 */
static void putc(char ch) 
{
	machine_debug_putc(ch);
}

/** Prints a string to the console. 
 *
 * @param str    String to be printed.
 * @param count  Number of characters to be printed.
 * @param unused Unused parameter.
 *
 * @return Number of printed characters.
 */
static int debug_write(const char *str, size_t count, void *unused)
{
	unsigned int i;
	for (i = 0; i < count; ++i)
		putc(str[i]);
	
	return i;
}

/** Prints a formated string.
 *
 * @param fmt "Printf-like" format.
 */
void debug_printf(const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);

	struct printf_spec ps = {
		(int (*)(void *, size_t, void *)) debug_write,
		NULL
	};
	printf_core(fmt, &ps, args);

	va_end(args);
}

/** Prints a string.
 *
 * @param str String to print.
 */
void debug_puts(const char *str)
{
	while (*str) {
		putc(*str);
		str++;
	}
}

/** @}
 */
