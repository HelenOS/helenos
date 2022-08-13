/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_KM_H_
#define KERN_riscv64_KM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KM_RISCV64_IDENTITY_START      UINT64_C(0xffff800000000000)
#define KM_RISCV64_IDENTITY_SIZE       UINT64_C(0x0000400000000000)

#define KM_RISCV64_NON_IDENTITY_START  UINT64_C(0xffffc00000000000)
#define KM_RISCV64_NON_IDENTITY_SIZE   UINT64_C(0x0000400000000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
