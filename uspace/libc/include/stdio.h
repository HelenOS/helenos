/*
 * Copyright (C) 2005 Martin Decky
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

#ifndef LIBC_STDIO_H_
#define LIBC_STDIO_H_

#include <types.h>
#include <stdarg.h>

#define EOF (-1)

#include <string.h>
#include <io/stream.h>

#define DEBUG(fmt, ...) \
{ \
	char buf[256]; \
	int n; \
	n = snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
	if (n > 0) \
		(void) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, strlen(buf)); \
}

extern int getchar(void);

extern int puts(const char * str);
extern int putchar(int c);

extern int printf(const char *fmt, ...);
extern int sprintf(char *str, const char *fmt, ...);
extern int snprintf(char *str, size_t size, const char *fmt, ...);

extern int vprintf(const char *fmt, va_list ap);
extern int vsprintf(char *str, const char *fmt, va_list ap);
extern int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

#define fprintf(f, fmt, ...) printf(fmt, ##__VA_ARGS__)

#endif

/** @}
 */
