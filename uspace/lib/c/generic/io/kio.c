/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2006 Jakub Vana
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
#include <str.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <abi/kio.h>
#include <io/kio.h>
#include <io/printf_core.h>

size_t kio_write(const void *buf, size_t size)
{
	ssize_t ret = (ssize_t) __SYSCALL3(SYS_KIO, KIO_WRITE, (sysarg_t) buf, size);
	
	if (ret >= 0)
		return (size_t) ret;
	
	return 0;
}

void kio_update(void)
{
	(void) __SYSCALL3(SYS_KIO, KIO_UPDATE, (uintptr_t) NULL, 0);
}

void kio_command(const void *buf, size_t size)
{
	(void) __SYSCALL3(SYS_KIO, KIO_COMMAND, (sysarg_t) buf, (sysarg_t) size);
}

/** Print formatted text to kio.
 *
 * @param fmt Format string
 *
 * \see For more details about format string see printf_core.
 *
 */
int kio_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	
	int ret = kio_vprintf(fmt, args);
	
	va_end(args);
	
	return ret;
}

static int kio_vprintf_str_write(const char *str, size_t size, void *data)
{
	size_t wr = kio_write(str, size);
	return str_nlength(str, wr);
}

static int kio_vprintf_wstr_write(const wchar_t *str, size_t size, void *data)
{
	size_t offset = 0;
	size_t chars = 0;
	
	while (offset < size) {
		char buf[STR_BOUNDS(1)];
		size_t sz = 0;
		
		if (chr_encode(str[chars], buf, &sz, STR_BOUNDS(1)) == EOK)
			kio_write(buf, sz);
		
		chars++;
		offset += sizeof(wchar_t);
	}
	
	return chars;
}

/** Print formatted text to kio.
 *
 * @param fmt Format string
 * @param ap  Format parameters
 *
 * \see For more details about format string see printf_core.
 *
 */
int kio_vprintf(const char *fmt, va_list ap)
{
	printf_spec_t ps = {
		kio_vprintf_str_write,
		kio_vprintf_wstr_write,
		NULL
	};
	
	return printf_core(fmt, &ps, ap);
}

/** @}
 */
