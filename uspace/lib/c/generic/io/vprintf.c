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

static errno_t vprintf_str_write(const char *str, size_t size, void *stream)
{
	errno_t old_errno = errno;

	errno = EOK;
	size_t written = fwrite(str, 1, size, (FILE *) stream);

	if (errno == EOK && written != size)
		errno = EIO;

	if (errno != EOK)
		return errno;

	errno = old_errno;
	return EOK;
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
