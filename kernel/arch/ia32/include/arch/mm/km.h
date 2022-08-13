/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_KM_H_
#define KERN_ia32_KM_H_

#include <stdbool.h>
#include <typedefs.h>

#define KM_IA32_IDENTITY_START		UINT32_C(0x80000000)
#define KM_IA32_IDENTITY_SIZE		UINT32_C(0x40000000)

#define KM_IA32_NON_IDENTITY_START	UINT32_C(0xc0000000)
#define KM_IA32_NON_IDENTITY_SIZE	UINT32_C(0x40000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
