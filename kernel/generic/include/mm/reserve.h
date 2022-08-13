/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_RESERVE_H_
#define KERN_RESERVE_H_

#include <stdbool.h>
#include <stddef.h>

extern void reserve_init(void);
extern bool reserve_try_alloc(size_t);
extern void reserve_force_alloc(size_t);
extern void reserve_free(size_t);

#endif

/** @}
 */
