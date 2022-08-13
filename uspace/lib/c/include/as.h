/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_AS_H_
#define _LIBC_AS_H_

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
extern errno_t as_area_get_info(void *, as_area_info_t *);
extern errno_t as_area_destroy(void *);
extern void *set_maxheapsize(size_t);
extern errno_t as_get_physical_mapping(const void *, uintptr_t *);

#endif

/** @}
 */
