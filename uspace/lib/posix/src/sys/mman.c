/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#include "../internal/common.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <as.h>
#include <unistd.h>

static int _prot_to_as(int prot)
{
	int ret = 0;

	if (prot & PROT_READ)
		ret |= AS_AREA_READ;

	if (prot & PROT_WRITE)
		ret |= AS_AREA_WRITE;

	if (prot & PROT_EXEC)
		ret |= AS_AREA_EXEC;

	return ret;
}

void *mmap(void *start, size_t length, int prot, int flags, int fd,
    off_t offset)
{
	if (!start)
		start = AS_AREA_ANY;

#if 0
	if (!((flags & MAP_SHARED) ^ (flags & MAP_PRIVATE)))
		return MAP_FAILED;
#endif

	if (!(flags & MAP_ANONYMOUS))
		return MAP_FAILED;

	return as_area_create(start, length, _prot_to_as(prot), AS_AREA_UNPAGED);
}

int munmap(void *start, size_t length)
{
	int rc = as_area_destroy(start);
	if (rc != EOK) {
		errno = rc;
		return -1;
	}
	return 0;
}

/** @}
 */
