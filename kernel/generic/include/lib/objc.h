/*
 * Copyright (c) 2006 Martin Decky
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

#ifndef KERN_OBJC_H_
#define KERN_OBJC_H_

#include <arch/types.h>
#include <arch/arg.h>

extern void *stderr;

extern void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function);
extern void abort(void);

extern void *fopen(const char *path, const char *mode);
extern size_t fread(void *ptr, size_t size, size_t nmemb, void *stream);
extern size_t fwrite(const void *ptr, size_t size, size_t nmemb, void *stream);
extern int fflush(void *stream);
extern int feof(void *stream);
extern int fclose(void *stream);

extern int vfprintf(void *stream, const char *format, va_list ap);
extern int sscanf(const char *str, const char *format, ...);
extern const unsigned short **__ctype_b_loc(void);
extern long int __strtol_internal(const char *__nptr, char **__endptr, int __base, int __group);

extern void *memset(void *s, int c, size_t n);
extern void *calloc(size_t nmemb, size_t size);

#endif

/** @}
 */
