/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_KM_H_
#define KERN_amd64_KM_H_

#include <stdbool.h>
#include <typedefs.h>

#ifdef MEMORY_MODEL_kernel

#define KM_AMD64_IDENTITY_START      UINT64_C(0xffffffff80000000)
#define KM_AMD64_IDENTITY_SIZE       UINT64_C(0x0000000080000000)

#define KM_AMD64_NON_IDENTITY_START  UINT64_C(0xffff800000000000)
#define KM_AMD64_NON_IDENTITY_SIZE   UINT64_C(0x00007fff80000000)

#endif /* MEMORY_MODEL_kernel */

#ifdef MEMORY_MODEL_large

#define KM_AMD64_IDENTITY_START      UINT64_C(0xffff800000000000)
#define KM_AMD64_IDENTITY_SIZE       UINT64_C(0x0000400000000000)

#define KM_AMD64_NON_IDENTITY_START  UINT64_C(0xffffc00000000000)
#define KM_AMD64_NON_IDENTITY_SIZE   UINT64_C(0x0000400000000000)

#endif /* MEMORY_MODEL_large */

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
