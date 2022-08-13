/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Declarations related to Restartable Atomic Sequences.
 */

#ifndef KERN_arm32_RAS_H_
#define KERN_arm32_RAS_H_

#include <arch/exception.h>
#include <typedefs.h>

#define RAS_START  0
#define RAS_END    1

extern uintptr_t *ras_page;

extern void ras_init(void);
extern void ras_check(unsigned int, istate_t *);

#endif

/** @}
 */
