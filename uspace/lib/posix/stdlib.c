/*
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include "stdlib.h"
#include "libc/sort.h"
#include "libc/str.h"
#include "libc/vfs/vfs.h"
#include "internal/common.h"
#include <errno.h>  // FIXME: use POSIX errno

/**
 * 
 * @param array
 * @param count
 * @param size
 * @param compare
 */
int posix_atexit(void (*func)(void))
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
int posix_abs(int i)
{
	return i < 0 ? -i : i;
}

/**
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long posix_labs(long i)
{
	return i < 0 ? -i : i;
}

/**
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long long posix_llabs(long long i)
{
	return i < 0 ? -i : i;
}

/**
 * Private helper function that serves as a compare function for qsort().
 *
 * @param elem1 First element to compare.
 * @param elem2 Second element to compare.
 * @param compare Comparison function without userdata parameter.
 *
 * @return Relative ordering of the elements.
 */
static int sort_compare_wrapper(void *elem1, void *elem2, void *userdata)
{
	int (*compare)(const void *, const void *) = userdata;
	return compare(elem1, elem2);
}

/**
 * Array sorting utilizing the quicksort algorithm.
 *
 * @param array
 * @param count
 * @param size
 * @param compare
 */
void posix_qsort(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *))
{
	/* Implemented in libc with one extra argument. */
	qsort(array, count, size, sort_compare_wrapper, compare);
}

/**
 * Retrieve a value of the given environment variable.
 * Since HelenOS doesn't support env variables at the moment,
 * this function always returns NULL.
 *
 * @param name
 * @return Always NULL.
 */
char *posix_getenv(const char *name)
{
	return NULL;
}

/**
 * 
 * @param name
 * @param resolved
 * @return
 */
int posix_putenv(char *string)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/**
 * 
 * @param name
 * @param resolved
 * @return
 */
char *posix_realpath(const char *name, char *resolved)
{
	#ifndef PATH_MAX
		assert(resolved == NULL);
	#endif
	
	if (name == NULL) {
		errno = EINVAL;
		return NULL;
	}
	
	// TODO: symlink resolution
	
	/* Function absolutize is implemented in libc and declared in vfs.h.
	 * No more processing is required as HelenOS doesn't have symlinks
	 * so far (as far as I can tell), although this function will need
	 * to be updated when that support is implemented.
	 */
	char* absolute = absolutize(name, NULL);
	
	if (absolute == NULL) {
		/* POSIX requires some specific errnos to be set
		 * for some cases, but there is no way to find out from
		 * absolutize().
		 */
		errno = EINVAL;
		return NULL;
	}
	
	if (resolved == NULL) {
		return absolute;
	} else {
		#ifdef PATH_MAX
			str_cpy(resolved, PATH_MAX, absolute);
		#endif
		free(absolute);
		return resolved;
	}
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See posix_strtold().
 *
 * @param nptr
 * @return
 */
double posix_atof(const char *nptr)
{
	return posix_strtod(nptr, NULL);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See posix_strtold().
 *
 * @param nptr
 * @param endptr
 * @return
 */
float posix_strtof(const char *restrict nptr, char **restrict endptr)
{
	return (float) posix_strtold(nptr, endptr);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See posix_strtold().
 *
 * @param nptr
 * @param endptr
 * @return
 */
double posix_strtod(const char *restrict nptr, char **restrict endptr)
{
	return (double) posix_strtold(nptr, endptr);
}

/**
 *
 * @param size
 * @return
 */
void *posix_malloc(size_t size)
{
	return malloc(size);
}

/**
 *
 * @param nelem
 * @param elsize
 * @return
 */
void *posix_calloc(size_t nelem, size_t elsize)
{
	return calloc(nelem, elsize);
}

/**
 *
 * @param ptr
 * @param size
 * @return
 */
void *posix_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

/**
 *
 * @param ptr
 */
void posix_free(void *ptr)
{
	free(ptr);
}

/**
 * 
 * @param tmpl
 * @return
 */
char *posix_mktemp(char *tmpl)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/** @}
 */
