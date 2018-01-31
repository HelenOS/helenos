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

/** @addtogroup libposix
 * @{
 */
/** @file Helper definitions common for all libposix files.
 */

#ifndef LIBPOSIX_COMMON_H_
#define LIBPOSIX_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <offset.h>
#include <vfs/vfs.h>

#define not_implemented() do { \
		static int __not_implemented_counter = 0; \
		if (__not_implemented_counter == 0) { \
			fprintf(stderr, "%s() not implemented in %s:%d, something will NOT work.\n", \
				__func__, __FILE__, __LINE__); \
		} \
		__not_implemented_counter++; \
	} while (0)

// Just a marker for external tools.
#define _HIDE_LIBC_SYMBOL(symbol)

/* Checks if the value is a failing error code.
 * If so, writes the error code to errno and returns true.
 */
static inline bool failed(int rc) {
	if (rc != EOK) {
		errno = rc;
		return true;
	}
	return false;
}

extern aoff64_t posix_pos[MAX_OPEN_FILES];

#endif /* LIBPOSIX_COMMON_H_ */

/** @}
 */
