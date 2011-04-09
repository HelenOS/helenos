/*
 * Copyright (c) 2011 Jiri Zarevucky
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

#ifndef COMPAT_STRING_H
#define COMPAT_STRING_H

#include <mem.h>
#include <str.h>

/// available in str.h
//
// char *strtok(char * /* restrict */ s1, const char * /* restrict */ s2);

/// available in mem.h
//
// void *memset(void *, int, size_t);
// void *memcpy(void *, const void *, size_t);
// void *memmove(void *, const void *, size_t);

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

extern char *strcpy(char * /* restrict */ dest, const char * /* restrict */ src);
extern char *strncpy(char * /* restrict */ dest, const char * /* restrict */ src, size_t n);

extern char *strcat(char * /* restrict */ dest, const char * /* restrict */ src);
extern char *strncat(char * /* restrict */ dest, const char * /* restrict */ src, size_t n);

extern int memcmp(const void *mem1, const void *mem2, size_t n);
extern void *memchr(const void *mem, int c, size_t n);

extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, size_t n);
extern int strcoll(const char *s1, const char *s2);
extern size_t strxfrm(char * /* restrict */ s1, const char * /* restrict */ s2, size_t n);

extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strpbrk(const char *s1, const char *s2);
extern size_t strcspn(const char *s1, const char *s2);
extern size_t strspn(const char *s1, const char *s2);
extern char *strstr(const char *s1, const char *s2);

extern char *strerror(int errnum);

extern size_t strlen(const char *s);

#endif  // COMPAT_STRING_H

