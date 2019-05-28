/*
 * Copyright (c) 2006 Ondrej Palkovsky
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
