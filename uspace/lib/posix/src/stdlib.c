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
#include <stdlib.h>

#include <errno.h>
#include <tmpfile.h>

#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <qsort.h>
#include <str.h>
#include <vfs/vfs.h>
#include <stats.h>

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

	/*
	 * Function absolutize is implemented in libc and declared in vfs.h.
	 * No more processing is required as HelenOS doesn't have symlinks
	 * so far (as far as I can tell), although this function will need
	 * to be updated when that support is implemented.
	 */
	char *absolute = vfs_absolutize(name, NULL);

	if (absolute == NULL) {
		/*
		 * POSIX requires some specific errnos to be set
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
	int tmpl_len;
	int file;

	tmpl_len = strlen(tmpl);
	if (tmpl_len < 6) {
		errno = EINVAL;
		return -1;
	}

	char *tptr = tmpl + tmpl_len - 6;
	if (strcmp(tptr, "XXXXXX") != 0) {
		errno = EINVAL;
		return -1;
	}

	file = __tmpfile_templ(tmpl, true);
	if (file < 0) {
		errno = EIO; // XXX could be more specific
		return -1;
	}

	return file;
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
	int tmpl_len;
	int rc;

	tmpl_len = strlen(tmpl);
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

	rc = __tmpfile_templ(tmpl, false);
	if (rc != 0) {
		errno = EIO; // XXX could be more specific
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
