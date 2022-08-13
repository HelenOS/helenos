/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_AS_H_
#define _ABI_AS_H_

#include <abi/cap.h>

/** Address space area flags. */
enum {
	AS_AREA_READ         = 0x01,
	AS_AREA_WRITE        = 0x02,
	AS_AREA_EXEC         = 0x04,
	AS_AREA_CACHEABLE    = 0x08,
	AS_AREA_GUARD        = 0x10,
	AS_AREA_LATE_RESERVE = 0x20,
};

static void *const AS_AREA_ANY = (void *) -1;
static void *const AS_MAP_FAILED = (void *) -1;
static void *const AS_AREA_UNPAGED = NULL;

/** Address space area info exported to uspace. */
typedef struct {
	/** Starting address */
	uintptr_t start_addr;

	/** Area size */
	size_t size;

	/** Area flags */
	unsigned int flags;
} as_area_info_t;

typedef struct {
	cap_phone_handle_t pager;
	sysarg_t id1;
	sysarg_t id2;
	sysarg_t id3;
} as_area_pager_info_t;

#endif

/** @}
 */
