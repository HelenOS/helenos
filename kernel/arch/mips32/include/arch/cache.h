/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_CACHE_H_
#define KERN_mips32_CACHE_H_

#include <arch/exception.h>

extern void cache_error(istate_t *istate);

#endif

/** @}
 */
