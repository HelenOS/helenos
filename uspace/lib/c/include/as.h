/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_AS_H_
#define LIBC_AS_H_

#include <types/common.h>
#include <stddef.h>
#include <stdint.h>
#include <abi/mm/as.h>
#include <libarch/config.h>

static inline size_t SIZE2PAGES(size_t size)
{
	if (size == 0)
		return 0;
	
	return (size_t) ((size - 1) >> PAGE_WIDTH) + 1;
}

static inline size_t PAGES2SIZE(size_t pages)
{
	return (size_t) (pages << PAGE_WIDTH);
}

extern void *as_area_create(void *, size_t, unsigned int,
    as_area_pager_info_t *);
extern errno_t as_area_resize(void *, size_t, unsigned int);
extern errno_t as_area_change_flags(void *, unsigned int);
extern errno_t as_area_destroy(void *);
extern void *set_maxheapsize(size_t);
extern errno_t as_get_physical_mapping(const void *, uintptr_t *);

#endif

/** @}
 */
