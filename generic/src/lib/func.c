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

#include <func.h>
#include <print.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <typedefs.h>

__u32	haltstate = 0; /**< Halt flag */


/** Halt wrapper
 *
 * Set halt flag and halt the cpu.
 *
 */
void halt(void)
{
	haltstate = 1;
	interrupts_disable();
	if (CPU)
		printf("cpu%d: halted\n", CPU->id);
	else
		printf("cpu: halted\n");
	cpu_halt();
}

/** Return number of characters in a string.
 *
 * @param str NULL terminated string.
 *
 * @return Number of characters in str.
 */
size_t strlen(const char *str)
{
	int i;
	
	for (i = 0; str[i]; i++)
		;
	
	return i;
}

/** Compare two NULL terminated strings
 *
 * Do a char-by-char comparison of two NULL terminated strings.
 * The strings are considered equal iff they consist of the same
 * characters on the minimum of their lengths and specified maximal
 * length.
 *
 * @param src First string to compare.
 * @param dst Second string to compare.
 * @param len Maximal length for comparison.
 *
 * @return 0 if the strings are equal, 1 otherwise.
 *
 */
int strncmp(const char *src, const char *dst, size_t len)
{
	int i;
	
	i = 0;
	while ((i < len) && (src[i] == dst[i])) {
		if ((i == len - 1) || (src[i] == '\0'))
			return 0;
		i++;
	}
	return 1;
}

/** Copy NULL terminated string.
 *
 * Copy at most 'len' characters from string 'src' to 'dest'.
 * If 'src' is shorter than 'len', '\0' is inserted behind the
 * last copied character.
 *
 * @param src Source string.
 * @param dst Destination buffer.
 * @param len Size of destination buffer.
 */
void strncpy(char *dest, const char *src, size_t len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (!(dest[i] = src[i]))
			return;
	}
}
