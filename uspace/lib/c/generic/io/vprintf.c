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
#include <printf_core.h>
#include <fibril_synch.h>
#include <async.h>
#include <str.h>

static FIBRIL_MUTEX_INITIALIZE(printf_mutex);

static int vprintf_str_write(const char *str, size_t size, void *stream)
{
	size_t wr = fwrite(str, 1, size, (FILE *) stream);
	return str_nlength(str, wr);
}

static int vprintf_wstr_write(const char32_t *str, size_t size, void *stream)
{
	size_t offset = 0;
	size_t chars = 0;

	while (offset < size) {
		if (fputuc(str[chars], (FILE *) stream) <= 0)
			break;

		chars++;
		offset += sizeof(char32_t);
	}

	return chars;
}

/** Print formatted text.
 *
 * @param stream Output stream
 * @param fmt    Format string
 * @param ap     Format parameters
 *
 * \see For more details about format string see printf_core.
 *
 */
int vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	printf_spec_t ps = {
		vprintf_str_write,
		vprintf_wstr_write,
		stream
	};

	/*
	 * Prevent other threads to execute printf_core()
	 */
	fibril_mutex_lock(&printf_mutex);

	int ret = printf_core(fmt, &ps, ap);

	fibril_mutex_unlock(&printf_mutex);

	return ret;
}

/** Print formatted text to stdout.
 *
 * @param fmt Format string
 * @param ap  Format parameters
 *
 * \see For more details about format string see printf_core.
 *
 */
int vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

/** @}
 */
