/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_arm64_KM_H_
#define KERN_arm64_KM_H_

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <typedefs.h>

#define KM_ARM64_IDENTITY_START  UINT64_C(0xffffffff00000000)
#define KM_ARM64_IDENTITY_SIZE   UINT64_C(0x0000000100000000)

#define KM_ARM64_NON_IDENTITY_START  UINT64_C(0xffff000000000000)
#define KM_ARM64_NON_IDENTITY_SIZE   UINT64_C(0x0000ffff00000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#else /* __ASSEMBLER__ */

#define KM_ARM64_IDENTITY_START  0xffffffff00000000
#define KM_ARM64_IDENTITY_SIZE   0x0000000100000000

#define KM_ARM64_NON_IDENTITY_START  0xffff000000000000
#define KM_ARM64_NON_IDENTITY_SIZE   0x0000ffff00000000

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
