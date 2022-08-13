/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_STDLIB_H_
#define KERN_STDLIB_H_

#include <stddef.h>

extern void *malloc(size_t)
    __attribute__((malloc));
extern void *realloc(void *, size_t)
    __attribute__((warn_unused_result));
extern void free(void *);

#endif

/** @}
 */
