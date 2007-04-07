/*
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <io/printf_core.h>

struct vsnprintf_data {
	size_t size; /* total space for string */
	size_t len; /* count of currently used characters */
	char *string; /* destination string */
};

/** Write string to given buffer.
 * Write at most data->size characters including trailing zero. According to C99 has snprintf to return number
 * of characters that would have been written if enough space had been available. Hence the return value is not
 * number of really printed characters but size of input string. Number of really used characters 
 * is stored in data->len.
 * @param str source string to print
 * @param count size of source string
 * @param data structure with destination string, counter of used space and total string size.
 * @return number of characters to print (not characters really printed!)
 */
static int vsnprintf_write(const char *str, size_t count, struct vsnprintf_data *data)
{
	size_t i;
	i = data->size - data->len;

	if (i == 0) {
		return count;
	}
	
	if (i == 1) {
		/* We have only one free byte left in buffer => write there trailing zero */
		data->string[data->size - 1] = 0;
		data->len = data->size;
		return count;
	}
	
	if (i <= count) {
		/* We have not enought space for whole string with the trailing zero => print only a part of string */
			memcpy((void *)(data->string + data->len), (void *)str, i - 1);
			data->string[data->size - 1] = 0;
			data->len = data->size;
			return count;
	}
	
	/* Buffer is big enought to print whole string */
	memcpy((void *)(data->string + data->len), (void *)str, count);
	data->len += count;
	/* Put trailing zero at end, but not count it into data->len so it could be rewritten next time */
	data->string[data->len] = 0;

	return count;	
}

/** Print formatted to the given buffer with limited size.
 * @param str	buffer
 * @param size	buffer size
 * @param fmt	format string
 * \see For more details about format string see printf_core.
 */
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	struct vsnprintf_data data = {size, 0, str};
	struct printf_spec ps = {(int(*)(void *, size_t, void *)) vsnprintf_write, &data};

	/* Print 0 at end of string - fix the case that nothing will be printed */
	if (size > 0)
		str[0] = 0;
	
	/* vsnprintf_write ensures that str will be terminated by zero. */
	return printf_core(fmt, &ps, ap);
}

/** @}
 */
