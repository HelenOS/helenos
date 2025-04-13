/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2025 Jiří Zárevúcky
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

#include <errno.h>
#include <macros.h>
#include <printf_core.h>
#include <stdarg.h>
#include <stdio.h>
#include <str.h>

typedef struct {
	char *dst;      /* Destination */
	size_t left;
} vsnprintf_data_t;

static int vsnprintf_str_write(const char *str, size_t size, void *data)
{
	vsnprintf_data_t *d = data;
	size_t left = min(size, d->left);
	if (left > 0) {
		memcpy(d->dst, str, left);
		d->dst += left;
		d->left -= left;
	}
	return EOK;
}

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	vsnprintf_data_t data = {
		.dst = str,
		.left = size ? size - 1 : 0,
	};

	printf_spec_t ps = {
		vsnprintf_str_write,
		&data
	};

	int written = printf_core(fmt, &ps, ap);
	if (written < 0)
		return written;

	/* Write the terminating NUL character. */
	if (size > 0)
		data.dst[0] = 0;

	return written;
}

/** @}
 */
