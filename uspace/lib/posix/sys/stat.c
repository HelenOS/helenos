/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

#include "stat.h"
#include <mem.h>

/**
 * Convert HelenOS stat struct into POSIX stat struct (if possible)
 *
 * @param dest
 * @param src
 */
static void stat_to_posix(struct posix_stat *dest, struct stat *src)
{
	memset(dest, 0, sizeof(struct posix_stat));
	
	dest->st_dev = src->device;
	
	/* HelenOS doesn't support permissions, so we set them all */
	dest->st_mode = S_IRWXU | S_IRWXG | S_IRWXO;
	if (src->is_file) {
		dest->st_mode |= S_IFREG;
	}
	if (src->is_directory) {
		dest->st_mode |= S_IFDIR;
	}
	
	dest->st_nlink = src->lnkcnt;
	dest->st_size = src->size;
}

/**
 *
 * @param fd
 * @param st
 * @return
 */
int posix_fstat(int fd, struct posix_stat *st)
{
	struct stat hst;
	if (fstat(fd, &hst) == -1) {
		// TODO: propagate a POSIX compatible errno
		return -1;
	}
	stat_to_posix(st, &hst);
	return 0;
}

/**
 *
 * @param path
 * @param st
 * @return
 */
int posix_stat(const char *path, struct posix_stat *st)
{
	struct stat hst;
	if (stat(path, &hst) == -1) {
		// TODO: propagate a POSIX compatible errno
		return -1;
	}
	stat_to_posix(st, &hst);
	return 0;
}

/** @}
 */
