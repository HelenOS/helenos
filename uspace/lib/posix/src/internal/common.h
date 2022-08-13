/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <errno.h>
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

/** Checks if the value is a failing error code.
 *
 * If so, writes the error code to errno and returns true.
 */
static inline bool failed(int rc)
{
	if (rc != EOK) {
		errno = rc;
		return true;
	}
	return false;
}

// TODO: Remove this arbitrary limit.
#define VFS_MAX_OPEN_FILES 128

extern aoff64_t posix_pos[VFS_MAX_OPEN_FILES];

#endif /* LIBPOSIX_COMMON_H_ */

/** @}
 */
