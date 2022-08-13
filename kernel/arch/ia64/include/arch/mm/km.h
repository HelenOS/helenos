/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_KM_H_
#define KERN_ia64_KM_H_

#include <stdbool.h>
#include <typedefs.h>

/*
 * Be conservative and assume the minimal (3 + 51)-bit virtual address width
 * of the Itanium CPU even if running on CPU with full 64-bit virtual
 * address width, such as Itanium 2.
 */

#define KM_IA64_IDENTITY_START		UINT64_C(0xe000000000000000)
#define KM_IA64_IDENTITY_SIZE		UINT64_C(0x0004000000000000)

#define KM_IA64_NON_IDENTITY_START	UINT64_C(0xfffc000000000000)
#define KM_IA64_NON_IDENTITY_SIZE	UINT64_C(0x0004000000000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
