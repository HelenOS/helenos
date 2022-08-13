/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_KM_H_
#define KERN_sparc64_sun4v_KM_H_

#include <stdbool.h>
#include <typedefs.h>

/*
 * Do not use the 4 GiB area on either side of the VA hole to meet the
 * limitations of the UltraSPARC T1 CPU.
 */

#define KM_SPARC64_T1_IDENTITY_START		UINT64_C(0x0000000000000000)
#define KM_SPARC64_T1_IDENTITY_SIZE		UINT64_C(0x00007fff00000000)

#define KM_SPARC64_T1_NON_IDENTITY_START	UINT64_C(0xffff800100000000)
#define KM_SPARC64_T1_NON_IDENTITY_SIZE		UINT64_C(0x00007fff00000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
