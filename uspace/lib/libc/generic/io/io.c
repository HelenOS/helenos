/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */ 

#include <libc.h>
#include <unistd.h>
#include <stdio.h>
#include <io/io.h>

const static char nl = '\n';

int puts(const char *str)
{
	size_t count;
	
	if (str == NULL)
		return putnchars("(NULL)", 6);
	
	for (count = 0; str[count] != 0; count++);
	if (write_stdout((void *) str, count) == count) {
		if (write_stdout(&nl, 1) == 1)
			return 0;
	}
	
	return EOF;
}

/** Put count chars from buffer to stdout without adding newline
 * @param buf Buffer with size at least count bytes - NULL pointer NOT allowed!
 * @param count 
 * @return 0 on succes, EOF on fail
 */
int putnchars(const char *buf, size_t count)
{
	if (write_stdout((void *) buf, count) == count)
		return 0;
	
	return EOF;
}

/** Same as puts, but does not print newline at end
 *
 */
int putstr(const char *str)
{
	size_t count;
	
	if (str == NULL)
		return putnchars("(NULL)", 6);

	for (count = 0; str[count] != 0; count++);
	if (write_stdout((void *) str, count) == count)
		return 0;
	
	return EOF;
}

int putchar(int c)
{
	unsigned char ch = c;
	if (write_stdout((void *) &ch, 1) == 1)
		return c;
	
	return EOF;
}

int getchar(void)
{
	unsigned char c;
	if (read_stdin((void *) &c, 1) == 1)
		return c;
	
	return EOF;
}

/** @}
 */
