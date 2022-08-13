/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_TYPES_H_
#define KERN_ia64_TYPES_H_

#include <_bits/all.h>

typedef struct {
	sysarg_t fnc;
	sysarg_t gp;
} __attribute__((may_alias)) fncptr_t;

#endif

/** @}
 */
