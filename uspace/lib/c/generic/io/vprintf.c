/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <stdarg.h>
#include <stdio.h>
#include <io/printf_core.h>
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
