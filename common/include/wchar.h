/*
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

#ifndef _LIBC_WCHAR_H_
#define _LIBC_WCHAR_H_

#include <_bits/NULL.h>
#include <_bits/WEOF.h>
#include <_bits/mbstate_t.h>
#include <_bits/size_t.h>
#include <_bits/wchar_limits.h>
#include <_bits/wchar_t.h>
#include <_bits/wint_t.h>
#include <stdarg.h>

/* Forward declarations for types. */
typedef struct _IO_FILE FILE;
struct tm;

/*
 * Complete list of functions straight out of ISO/IEC 9899:2023 (N3096 draft).
 * Only some of these are implemented, but keep all standard prototypes here
 * for less pain in libposix etc.
 */

int fputws(const wchar_t *__restrict__, FILE *__restrict__);
int fwide(FILE *, int mode);
int fwprintf(FILE *__restrict__, const wchar_t *__restrict__, ...);
int fwscanf(FILE *__restrict__, const wchar_t *__restrict__, ...);
int mbsinit(const mbstate_t *);
int swprintf(wchar_t *__restrict__, size_t, const wchar_t *__restrict__, ...);
int swscanf(const wchar_t *__restrict__, const wchar_t *__restrict__, ...);
int vfwprintf(FILE *__restrict__, const wchar_t *__restrict__, va_list);
int vfwscanf(FILE *__restrict__, const wchar_t *__restrict__, va_list);
int vswprintf(wchar_t *__restrict__, size_t, const wchar_t *__restrict__, va_list);
int vswscanf(const wchar_t *__restrict__, const wchar_t *__restrict__, va_list);
int vwprintf(const wchar_t *__restrict__, va_list);
int vwscanf(const wchar_t *__restrict__, va_list);
int wcscmp(const wchar_t *, const wchar_t *);
int wcscoll(const wchar_t *, const wchar_t *);
int wcsncmp(const wchar_t *, const wchar_t *, size_t);
int wctob(wint_t);
int wmemcmp(const wchar_t *, const wchar_t *, size_t);
int wprintf(const wchar_t *__restrict__, ...);
int wscanf(const wchar_t *__restrict__, ...);
long int wcstol(const wchar_t *__restrict__, wchar_t **__restrict__, int);
long long int wcstoll(const wchar_t *__restrict__, wchar_t **__restrict__, int);
size_t mbrlen(const char *__restrict__, size_t, mbstate_t *__restrict__);
size_t mbrtowc(wchar_t *__restrict__ pwc, const char *__restrict__, size_t, mbstate_t *__restrict__);
size_t mbsrtowcs(wchar_t *__restrict__ dst, const char **__restrict__ src, size_t, mbstate_t *__restrict__);
size_t wcrtomb(char *__restrict__, wchar_t wc, mbstate_t *__restrict__);
size_t wcscspn(const wchar_t *, const wchar_t *);
size_t wcsftime(wchar_t *__restrict__, size_t maxsize, const wchar_t *__restrict__, const struct tm *__restrict__ timeptr);
size_t wcslen(const wchar_t *);
size_t wcsrtombs(char *__restrict__ dst, const wchar_t **__restrict__ src, size_t, mbstate_t *__restrict__);
size_t wcsspn(const wchar_t *, const wchar_t *);
size_t wcsxfrm(wchar_t *__restrict__, const wchar_t *__restrict__, size_t);
unsigned long int wcstoul(const wchar_t *__restrict__, wchar_t **__restrict__, int);
unsigned long long int wcstoull(const wchar_t *__restrict__, wchar_t **__restrict__, int);
wchar_t *fgetws(wchar_t *__restrict__, int, FILE *__restrict__);
wchar_t *wcscat(wchar_t *__restrict__, const wchar_t *__restrict__);
wchar_t *wcscpy(wchar_t *__restrict__, const wchar_t *__restrict__);
wchar_t *wcsncat(wchar_t *__restrict__, const wchar_t *__restrict__, size_t);
wchar_t *wcsncpy(wchar_t *__restrict__, const wchar_t *__restrict__, size_t);
wchar_t *wcstok(wchar_t *__restrict__, const wchar_t *__restrict__, wchar_t **__restrict__);
wchar_t *wmemcpy(wchar_t *__restrict__, const wchar_t *__restrict__, size_t);
wchar_t *wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t *wmemset(wchar_t *, wchar_t, size_t);
wint_t btowc(int);
wint_t fgetwc(FILE *);
wint_t fputwc(wchar_t, FILE *);
wint_t getwc(FILE *);
wint_t getwchar(void);
wint_t putwc(wchar_t, FILE *);
wint_t putwchar(wchar_t);
wint_t ungetwc(wint_t, FILE *);

// TODO: In C23, these functions have generic macro definitions for correct
//       const handling.
wchar_t *wcschr(const wchar_t *, wchar_t);
wchar_t *wcspbrk(const wchar_t *, const wchar_t *);
wchar_t *wcsrchr(const wchar_t *, wchar_t);
wchar_t *wcsstr(const wchar_t *, const wchar_t *);
wchar_t *wmemchr(const wchar_t *, wchar_t, size_t);

#endif /* _LIBC_WCHAR_H_ */
