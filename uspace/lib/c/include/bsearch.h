/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_BSEARCH_H_
#define _LIBC_BSEARCH_H_

#include <stddef.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

extern void *bsearch(const void *, const void *, size_t, size_t,
    int (*)(const void *, const void *));

__C_DECLS_END;

#endif

/** @}
 */
