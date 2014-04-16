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
/** @file Standard library definitions.
 */

#define LIBPOSIX_INTERNAL
#define __POSIX_DEF__(x) posix_##x

#include "internal/common.h"
#include "posix/stdlib.h"

#include "posix/errno.h"
#include "posix/fcntl.h"
#include "posix/limits.h"
#include "posix/string.h"
#include "posix/sys/stat.h"
#include "posix/unistd.h"

#include "libc/sort.h"
#include "libc/str.h"
#include "libc/vfs/vfs.h"
#include "libc/stats.h"

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
	return 0;
}

/**
 * Integer absolute value.
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
int posix_abs(int i)
{
	return i < 0 ? -i : i;
}

/**
 * Long integer absolute value.
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long posix_labs(long i)
{
	return i < 0 ? -i : i;
}

/**
 * Long long integer absolute value.
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long long posix_llabs(long long i)
{
	return i < 0 ? -i : i;
}

/**
 * Compute the quotient and remainder of an integer division.
 *
 * @param numer Numerator.
 * @param denom Denominator.
 * @return Quotient and remainder packed into structure.
 */
posix_div_t posix_div(int numer, int denom)
{
	return (posix_div_t) { .quot = numer / denom, .rem = numer % denom };
}

/**
 * Compute the quotient and remainder of a long integer division.
 *
 * @param numer Numerator.
 * @param denom Denominator.
 * @return Quotient and remainder packed into structure.
 */
posix_ldiv_t posix_ldiv(long numer, long denom)
{
	return (posix_ldiv_t) { .quot = numer / denom, .rem = numer % denom };
}

/**
 * Compute the quotient and remainder of a long long integer division.
 *
 * @param numer Numerator.
 * @param denom Denominator.
 * @return Quotient and remainder packed into structure.
 */
posix_lldiv_t posix_lldiv(long long numer, long long denom)
{
	return (posix_lldiv_t) { .quot = numer / denom, .rem = numer % denom };
}

/**
 * Private helper function that serves as a compare function for qsort().
 *
 * @param elem1 First element to compare.
 * @param elem2 Second element to compare.
 * @param compare Comparison function without userdata parameter.
 * @return Relative ordering of the elements.
 */
static int sort_compare_wrapper(void *elem1, void *elem2, void *userdata)
{
	int (*compare)(const void *, const void *) = userdata;
	int ret = compare(elem1, elem2);
	
	/* Native qsort internals expect this. */
	if (ret < 0) {
		return -1;
	} else if (ret > 0) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Array sorting utilizing the quicksort algorithm.
 *
 * @param array Array of elements to sort.
 * @param count Number of elements in the array.
 * @param size Width of each element.
 * @param compare Decides relative ordering of two elements.
 */
void posix_qsort(void *array, size_t count, size_t size,
    int (*compare)(const void *, const void *))
{
	/* Implemented in libc with one extra argument. */
	qsort(array, count, size, sort_compare_wrapper, compare);
}

/**
 * Binary search in a sorted array.
 *
 * @param key Object to search for.
 * @param base Pointer to the first element of the array.
 * @param nmemb Number of elements in the array.
 * @param size Size of each array element.
 * @param compar Comparison function.
 * @return Pointer to a matching element, or NULL if none can be found.
 */
void *posix_bsearch(const void *key, const void *base,
    size_t nmemb, size_t size, int (*compar)(const void *, const void *))
{
	while (nmemb > 0) {
		const void *middle = base + (nmemb / 2) * size;
		int cmp = compar(key, middle);
		if (cmp == 0) {
			return (void *) middle;
		}
		if (middle == base) {
			/* There is just one member left to check and it
			 * didn't match the key. Avoid infinite loop.
			 */
			break;
		}
		if (cmp < 0) {
			nmemb = nmemb / 2;
		} else if (cmp > 0) {
			nmemb = nmemb - (nmemb / 2);
			base = middle;
		}
	}
	
	return NULL;
}

/**
 * Retrieve a value of the given environment variable.
 *
 * Since HelenOS doesn't support env variables at the moment,
 * this function always returns NULL.
 *
 * @param name Name of the variable.
 * @return Value of the variable or NULL if such variable does not exist.
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
	return 0;
}

/**
 * Issue a command.
 *
 * @param string String to be passed to a command interpreter or NULL.
 * @return Termination status of the command if the command is not NULL,
 *     otherwise indicate whether there is a command interpreter (non-zero)
 *     or not (zero).
 */
int posix_system(const char *string) {
	// TODO: does nothing at the moment
	not_implemented();
	return 0;
}

/**
 * Resolve absolute pathname.
 * 
 * @param name Pathname to be resolved.
 * @param resolved Either buffer for the resolved absolute pathname or NULL.
 * @return On success, either resolved (if it was not NULL) or pointer to the
 *     newly allocated buffer containing the absolute pathname (if resolved was
 *     NULL). Otherwise NULL.
 *
 */
char *posix_realpath(const char *restrict name, char *restrict resolved)
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
 * @param nptr String representation of a floating-point number.
 * @return Double-precision number resulting from the string conversion.
 */
double posix_atof(const char *nptr)
{
	return posix_strtod(nptr, NULL);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See posix_strtold().
 *
 * @param nptr String representation of a floating-point number.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @return Single-precision number resulting from the string conversion.
 */
float posix_strtof(const char *restrict nptr, char **restrict endptr)
{
	return (float) posix_strtold(nptr, endptr);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See posix_strtold().
 *
 * @param nptr String representation of a floating-point number.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @return Double-precision number resulting from the string conversion.
 */
double posix_strtod(const char *restrict nptr, char **restrict endptr)
{
	return (double) posix_strtold(nptr, endptr);
}

/**
 * Allocate memory chunk.
 *
 * @param size Size of the chunk to allocate.
 * @return Either pointer to the allocated chunk or NULL if not possible.
 */
void *posix_malloc(size_t size)
{
	return malloc(size);
}

/**
 * Allocate memory for an array of elements.
 *
 * @param nelem Number of elements in the array.
 * @param elsize Size of each element.
 * @return Either pointer to the allocated array or NULL if not possible.
 */
void *posix_calloc(size_t nelem, size_t elsize)
{
	return calloc(nelem, elsize);
}

/**
 * Reallocate memory chunk to a new size.
 *
 * @param ptr Memory chunk to reallocate. Might be NULL.
 * @param size Size of the reallocated chunk. Might be zero.
 * @return Either NULL or the pointer to the newly reallocated chunk.
 */
void *posix_realloc(void *ptr, size_t size)
{
	if (ptr != NULL && size == 0) {
		/* Native realloc does not handle this special case. */
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, size);
	}
}

/**
 * Free allocated memory chunk.
 *
 * @param ptr Memory chunk to be freed.
 */
void posix_free(void *ptr)
{
	if (ptr) {
		free(ptr);
	}
}

/**
 * Generate a pseudo random integer in the range 0 to RAND_MAX inclusive.
 *
 * @return The pseudo random integer.
 */
int posix_rand(void)
{
	return (int) random();
}

/**
 * Initialize a new sequence of pseudo-random integers.
 *
 * @param seed The seed of the new sequence.
 */
void posix_srand(unsigned int seed)
{
	srandom(seed);
}

/**
 * Creates and opens an unique temporary file from template.
 *
 * @param tmpl Template. Last six characters must be XXXXXX.
 * @return The opened file descriptor or -1 on error.
 */
int posix_mkstemp(char *tmpl)
{
	int fd = -1;
	
	char *tptr = tmpl + posix_strlen(tmpl) - 6;
	
	while (fd < 0) {
		if (*posix_mktemp(tmpl) == '\0') {
			/* Errno set by mktemp(). */
			return -1;
		}
		
		fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
		
		if (fd == -1) {
			/* Restore template to it's original state. */
			snprintf(tptr, 7, "XXXXXX");
		}
	}
	
	return fd;
}

/**
 * Creates an unique temporary file name from template.
 *
 * @param tmpl Template. Last six characters must be XXXXXX.
 * @return The value of tmpl. The template is modified in place.
 *    If no temporary file name can be created, template is
 *    reduced to an empty string.
 */
char *posix_mktemp(char *tmpl)
{
	int tmpl_len = posix_strlen(tmpl);
	if (tmpl_len < 6) {
		errno = EINVAL;
		*tmpl = '\0';
		return tmpl;
	}
	
	char *tptr = tmpl + tmpl_len - 6;
	if (posix_strcmp(tptr, "XXXXXX") != 0) {
		errno = EINVAL;
		*tmpl = '\0';
		return tmpl;
	}
	
	static int seq = 0;
	
	for (; seq < 1000000; ++seq) {
		snprintf(tptr, 7, "%06d", seq);
		
		int orig_errno = errno;
		errno = 0;
		/* Check if the file exists. */
		if (posix_access(tmpl, F_OK) == -1) {
			if (errno == ENOENT) {
				errno = orig_errno;
				break;
			} else {
				/* errno set by access() */
				*tmpl = '\0';
				return tmpl;
			}
		}
	}
	
	if (seq == 10000000) {
		errno = EEXIST;
		*tmpl = '\0';
		return tmpl;
	}
	
	return tmpl;
}

/**
 * Get system load average statistics.
 *
 * @param loadavg Array where the load averages shall be placed.
 * @param nelem Maximum number of elements to be placed into the array.
 * @return Number of elements placed into the array on success, -1 otherwise.
 */
int bsd_getloadavg(double loadavg[], int nelem)
{
	assert(nelem > 0);
	
	size_t count;
	load_t *loads = stats_get_load(&count);
	
	if (loads == NULL) {
		return -1;
	}
	
	if (((size_t) nelem) < count) {
		count = nelem;
	}
	
	for (size_t i = 0; i < count; ++i) {
		loadavg[i] = (double) loads[i];
	}
	
	free(loads);
	return count;
}

/** @}
 */
