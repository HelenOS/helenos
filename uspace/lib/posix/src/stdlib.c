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

#include "internal/common.h"
#include "posix/stdlib.h"

#include <errno.h>

#include "posix/fcntl.h"
#include "posix/limits.h"
#include "posix/string.h"
#include "posix/sys/stat.h"
#include "posix/unistd.h"

#include "libc/qsort.h"
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
int atexit(void (*func)(void))
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
int abs(int i)
{
	return i < 0 ? -i : i;
}

/**
 * Long integer absolute value.
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long labs(long i)
{
	return i < 0 ? -i : i;
}

/**
 * Long long integer absolute value.
 * 
 * @param i Input value.
 * @return Absolute value of the parameter.
 */
long long llabs(long long i)
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
div_t div(int numer, int denom)
{
	return (div_t) { .quot = numer / denom, .rem = numer % denom };
}

/**
 * Compute the quotient and remainder of a long integer division.
 *
 * @param numer Numerator.
 * @param denom Denominator.
 * @return Quotient and remainder packed into structure.
 */
ldiv_t ldiv(long numer, long denom)
{
	return (ldiv_t) { .quot = numer / denom, .rem = numer % denom };
}

/**
 * Compute the quotient and remainder of a long long integer division.
 *
 * @param numer Numerator.
 * @param denom Denominator.
 * @return Quotient and remainder packed into structure.
 */
lldiv_t lldiv(long long numer, long long denom)
{
	return (lldiv_t) { .quot = numer / denom, .rem = numer % denom };
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
void *bsearch(const void *key, const void *base,
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
char *getenv(const char *name)
{
	return NULL;
}

/**
 * 
 * @param name
 * @param resolved
 * @return
 */
int putenv(char *string)
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
int system(const char *string) {
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
char *realpath(const char *restrict name, char *restrict resolved)
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
	char* absolute = vfs_absolutize(name, NULL);
	
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
 * its native representation. See strtold().
 *
 * @param nptr String representation of a floating-point number.
 * @return Double-precision number resulting from the string conversion.
 */
double atof(const char *nptr)
{
	return strtod(nptr, NULL);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See strtold().
 *
 * @param nptr String representation of a floating-point number.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @return Single-precision number resulting from the string conversion.
 */
float strtof(const char *restrict nptr, char **restrict endptr)
{
	return (float) strtold(nptr, endptr);
}

/**
 * Converts a string representation of a floating-point number to
 * its native representation. See strtold().
 *
 * @param nptr String representation of a floating-point number.
 * @param endptr Pointer to the final part of the string which
 *     was not used for conversion.
 * @return Double-precision number resulting from the string conversion.
 */
double strtod(const char *restrict nptr, char **restrict endptr)
{
	return (double) strtold(nptr, endptr);
}

/**
 * Creates and opens an unique temporary file from template.
 *
 * @param tmpl Template. Last six characters must be XXXXXX.
 * @return The opened file descriptor or -1 on error.
 */
int mkstemp(char *tmpl)
{
	int fd = -1;
	
	char *tptr = tmpl + strlen(tmpl) - 6;
	
	while (fd < 0) {
		if (*mktemp(tmpl) == '\0') {
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
char *mktemp(char *tmpl)
{
	int tmpl_len = strlen(tmpl);
	if (tmpl_len < 6) {
		errno = EINVAL;
		*tmpl = '\0';
		return tmpl;
	}
	
	char *tptr = tmpl + tmpl_len - 6;
	if (strcmp(tptr, "XXXXXX") != 0) {
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
		if (access(tmpl, F_OK) == -1) {
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
