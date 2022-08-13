/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_QSORT_H_
#define _LIBC_QSORT_H_

#include <stddef.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;
extern void qsort(void *, size_t, size_t, int (*)(const void *,
    const void *));
__C_DECLS_END;

#ifdef _HELENOS_SOURCE
__HELENOS_DECLS_BEGIN;
extern void qsort_r(void *, size_t, size_t, int (*)(const void *,
    const void *, void *), void *);
__HELENOS_DECLS_END;
#endif

#endif

/** @}
 */
