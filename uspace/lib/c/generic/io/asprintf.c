/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2008 Jakub Jermar
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
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include <io/printf_core.h>

static int asprintf_str_write(const char *str, size_t count, void *unused)
{
	return str_nlength(str, count);
}

static int asprintf_wstr_write(const wchar_t *str, size_t count, void *unused)
{
	return wstr_nlength(str, count);
}

int vprintf_size(const char *fmt, va_list args)
{
	printf_spec_t ps = {
		asprintf_str_write,
		asprintf_wstr_write,
		NULL
	};
	
	return printf_core(fmt, &ps, args);
}

int printf_size(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vprintf_size(fmt, args);
	va_end(args);
	
	return ret;
}

/** Allocate and print to string.
 *
 * @param strp Address of the pointer where to store the address of
 *             the newly allocated string.
 * @fmt        Format string.
 * @args       Variable argument list
 *
 * @return Number of characters printed or an error code.
 *
 */
int vasprintf(char **strp, const char *fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	int ret = vprintf_size(fmt, args2);
	va_end(args2);
	
	if (ret > 0) {
		*strp = malloc(STR_BOUNDS(ret) + 1);
		if (*strp == NULL)
			return -1;
		
		vsnprintf(*strp, STR_BOUNDS(ret) + 1, fmt, args);
	}
	
	return ret;
}

/** Allocate and print to string.
 *
 * @param strp Address of the pointer where to store the address of
 *             the newly allocated string.
 * @fmt        Format string.
 *
 * @return Number of characters printed or an error code.
 *
 */
int asprintf(char **strp, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vasprintf(strp, fmt, args);
	va_end(args);
	
	return ret;
}

/** @}
 */
