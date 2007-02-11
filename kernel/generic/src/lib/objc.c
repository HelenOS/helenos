/*
 * Copyright (c) 2007 Martin Decky
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

/**
 * @file
 * @brief	Objective C bindings.
 *
 * This file provides architecture independent binding
 * functions which are needed to link with libobjc run-time
 * library. Many of the functions are just dummy.
 */

#include <lib/objc.h>
#include <panic.h>
#include <arch/memstr.h>
#include <mm/slab.h>

void *stderr;

static unsigned short __ctype_b[384] = {
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    2,     2,     2,     2,     2,     2,     2,     2, 
	    2,  8195,  8194,  8194,  8194,  8194,     2,     2, 
	    2,     2,     2,     2,     2,     2,     2,     2, 
	    2,     2,     2,     2,     2,     2,     2,     2, 
	24577, 49156, 49156, 49156, 49156, 49156, 49156, 49156, 
	49156, 49156, 49156, 49156, 49156, 49156, 49156, 49156, 
	55304, 55304, 55304, 55304, 55304, 55304, 55304, 55304, 
	55304, 55304, 49156, 49156, 49156, 49156, 49156, 49156, 
	49156, 54536, 54536, 54536, 54536, 54536, 54536, 50440, 
	50440, 50440, 50440, 50440, 50440, 50440, 50440, 50440, 
	50440, 50440, 50440, 50440, 50440, 50440, 50440, 50440, 
	50440, 50440, 50440, 49156, 49156, 49156, 49156, 49156, 
	49156, 54792, 54792, 54792, 54792, 54792, 54792, 50696, 
	50696, 50696, 50696, 50696, 50696, 50696, 50696, 50696, 
	50696, 50696, 50696, 50696, 50696, 50696, 50696, 50696, 
	50696, 50696, 50696, 49156, 49156, 49156, 49156,     2, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0, 
	    0,     0,     0,     0,     0,     0,     0,     0
};

static const unsigned short *__ctype_b_ptr = __ctype_b + 128;

void __assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
	panic("Run-time assertion (%s:%d:%s) failed (%s)", file, line, function ? function : "", assertion);
}

void abort(void)
{
	panic("Run-time scheduled abort");
}

void *fopen(const char *path, const char *mode)
{
	return NULL;
}

size_t fread(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, void *stream)
{
	return 0;
}

int fflush(void *stream)
{
	return 0;
}

int feof(void *stream)
{
	return 1;
}

int fclose(void *stream)
{
	return 0;
}

int vfprintf(void *stream, const char *format, va_list ap)
{
	return 0;
}

int sscanf(const char *str, const char *format, ...)
{
	return 0;
}

const unsigned short **__ctype_b_loc(void)
{
	return &__ctype_b_ptr;
}

long int __strtol_internal(const char *__nptr, char **__endptr, int __base, int __group)
{
	return 0;
}

void *memset(void *s, int c, size_t n)
{
	memsetb((uintptr_t) s, n, c);
	return s;
}

void *calloc(size_t nmemb, size_t size)
{
	return malloc(nmemb * size, 0);
}

/** @}
 */
