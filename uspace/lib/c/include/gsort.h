/*
 * SPDX-FileCopyrightText: 2005 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_SORT_H_
#define _LIBC_SORT_H_

#include <stddef.h>
#include <stdbool.h>

typedef int (*sort_cmp_t)(void *, void *, void *);

extern bool gsort(void *, size_t, size_t, sort_cmp_t, void *);

#endif

/** @}
 */
