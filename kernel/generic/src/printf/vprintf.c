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

/** @addtogroup generic
 * @{
 */
/** @file
 */

#include <print.h>
#include <printf/printf_core.h>
#include <putchar.h>
#include <synch/spinlock.h>
#include <arch/asm.h>
#include <arch/types.h>
#include <typedefs.h>
#include <string.h>

SPINLOCK_INITIALIZE(printf_lock);  /**< vprintf spinlock */

static int vprintf_write_utf8(const char *str, size_t size, void *data)
{
	index_t index = 0;
	index_t chars = 0;
	
	while (index < size) {
		putchar(utf8_decode(str, &index, size));
		chars++;
	}
	
	return chars;
}

static int vprintf_write_utf32(const wchar_t *str, size_t size, void *data)
{
	index_t index = 0;
	
	while (index < (size / sizeof(wchar_t))) {
		putchar(str[index]);
		index++;
	}
	
	return index;
}

int puts(const char *str)
{
	index_t index = 0;
	index_t chars = 0;
	wchar_t uc;
	
	while ((uc = utf8_decode(str, &index, UTF8_NO_LIMIT)) != 0) {
		putchar(uc);
		chars++;
	}
	
	return chars;
}

int vprintf(const char *fmt, va_list ap)
{
	printf_spec_t ps = {
		vprintf_write_utf8,
		vprintf_write_utf32,
		NULL
	};
	
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&printf_lock);
	
	int ret = printf_core(fmt, &ps, ap);
	
	spinlock_unlock(&printf_lock);
	interrupts_restore(ipl);
	
	return ret;
}

/** @}
 */
