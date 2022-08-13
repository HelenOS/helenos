/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_MALLOC_H_
#define _LIBC_MALLOC_H_

#include <stddef.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

extern void *malloc(size_t size)
    __attribute__((malloc));
extern void *calloc(size_t nmemb, size_t size)
    __attribute__((malloc));
extern void *realloc(void *addr, size_t size)
    __attribute__((warn_unused_result));
extern void free(void *addr);

__C_DECLS_END;

#ifdef _HELENOS_SOURCE
__HELENOS_DECLS_BEGIN;

extern void *memalign(size_t align, size_t size)
    __attribute__((malloc));
extern void *heap_check(void);

__HELENOS_DECLS_END;
#endif

#endif

/** @}
 */
