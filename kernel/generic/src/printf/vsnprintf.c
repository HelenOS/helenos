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

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <print.h>
#include <printf/printf_core.h>
#include <str.h>
#include <memw.h>
#include <errno.h>

typedef struct {
	size_t size;    /* Total size of the buffer (in bytes) */
	size_t len;     /* Number of already used bytes */
	char *dst;      /* Destination */
} vsnprintf_data_t;

/** Write string to given buffer.
 *
 * Write at most data->size plain characters including trailing zero.
 * According to C99, snprintf() has to return number of characters that
 * would have been written if enough space had been available. Hence
 * the return value is not the number of actually printed characters
 * but size of the input string.
 *
 * @param str  Source string to print.
 * @param size Number of plain characters in str.
 * @param data Structure describing destination string, counter
 *             of used space and total string size.
 *
 * @return Number of characters to print (not characters actually
 *         printed).
 *
 */
static int vsnprintf_str_write(const char *str, size_t size, vsnprintf_data_t *data)
{
	size_t left = data->size - data->len;

	if (left == 0)
		return ((int) size);

	if (left == 1) {
		/*
		 * We have only one free byte left in buffer
		 * -> store trailing zero
		 */
		data->dst[data->size - 1] = 0;
		data->len = data->size;
		return ((int) size);
	}

	if (left <= size) {
		/*
		 * We do not have enough space for the whole string
		 * with the trailing zero => print only a part
		 * of string
		 */
		size_t index = 0;

		while (index < size) {
			char32_t uc = str_decode(str, &index, size);

			if (chr_encode(uc, data->dst, &data->len, data->size - 1) != EOK)
				break;
		}

		/*
		 * Put trailing zero at end, but not count it
		 * into data->len so it could be rewritten next time
		 */
		data->dst[data->len] = 0;

		return ((int) size);
	}

	/* Buffer is big enough to print the whole string */
	memcpy((void *)(data->dst + data->len), (void *) str, size);
	data->len += size;

	/*
	 * Put trailing zero at end, but not count it
	 * into data->len so it could be rewritten next time
	 */
	data->dst[data->len] = 0;

	return ((int) size);
}

/** Write wide string to given buffer.
 *
 * Write at most data->size plain characters including trailing zero.
 * According to C99, snprintf() has to return number of characters that
 * would have been written if enough space had been available. Hence
 * the return value is not the number of actually printed characters
 * but size of the input string.
 *
 * @param str  Source wide string to print.
 * @param size Number of bytes in str.
 * @param data Structure describing destination string, counter
 *             of used space and total string size.
 *
 * @return Number of wide characters to print (not characters actually
 *         printed).
 *
 */
static int vsnprintf_wstr_write(const char32_t *str, size_t size, vsnprintf_data_t *data)
{
	size_t index = 0;

	while (index < (size / sizeof(char32_t))) {
		size_t left = data->size - data->len;

		if (left == 0)
			return ((int) size);

		if (left == 1) {
			/*
			 * We have only one free byte left in buffer
			 * -> store trailing zero
			 */
			data->dst[data->size - 1] = 0;
			data->len = data->size;
			return ((int) size);
		}

		if (chr_encode(str[index], data->dst, &data->len, data->size - 1) != EOK)
			break;

		index++;
	}

	/*
	 * Put trailing zero at end, but not count it
	 * into data->len so it could be rewritten next time
	 */
	data->dst[data->len] = 0;

	return ((int) size);
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	vsnprintf_data_t data = {
		size,
		0,
		str
	};
	printf_spec_t ps = {
		(int (*) (const char *, size_t, void *)) vsnprintf_str_write,
		(int (*) (const char32_t *, size_t, void *)) vsnprintf_wstr_write,
		&data
	};

	/* Print 0 at end of string - fix the case that nothing will be printed */
	if (size > 0)
		str[0] = 0;

	/* vsnprintf_write ensures that str will be terminated by zero. */
	return printf_core(fmt, &ps, ap);
}

/** @}
 */
