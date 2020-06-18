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
#include <putchar.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <typedefs.h>
#include <str.h>

static int vprintf_str_write(const char *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;

	while (offset < size) {
		putuchar(str_decode(str, &offset, size));
		chars++;
	}

	return chars;
}

static int vprintf_wstr_write(const char32_t *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;

	while (offset < size) {
		putuchar(str[chars]);
		chars++;
		offset += sizeof(char32_t);
	}

	return chars;
}

int puts(const char *str)
{
	size_t offset = 0;
	size_t chars = 0;
	char32_t uc;

	while ((uc = str_decode(str, &offset, STR_NO_LIMIT)) != 0) {
		putuchar(uc);
		chars++;
	}

	putuchar('\n');
	return chars;
}

int vprintf(const char *fmt, va_list ap)
{
	printf_spec_t ps = {
		vprintf_str_write,
		vprintf_wstr_write,
		NULL
	};

	return printf_core(fmt, &ps, ap);
}

/** @}
 */
