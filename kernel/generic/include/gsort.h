/*
 * SPDX-FileCopyrightText: 2005 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_GSORT_H_
#define KERN_GSORT_H_

#include <stdbool.h>
#include <stddef.h>

typedef int (*sort_cmp_t)(void *, void *, void *);

extern bool gsort(void *, size_t, size_t, sort_cmp_t, void *);

#endif

/** @}
 */
