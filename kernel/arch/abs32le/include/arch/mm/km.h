/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le_mm
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_KM_H_
#define KERN_abs32le_KM_H_

#include <stdbool.h>
#include <typedefs.h>

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
