/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_KM_H_
#define KERN_mips32_KM_H_

#include <stdbool.h>
#include <typedefs.h>

#define KM_MIPS32_KSEG0_START	UINT32_C(0x80000000)
#define KM_MIPS32_KSEG0_SIZE	UINT32_C(0x20000000)

#define KM_MIPS32_KSSEG_START	UINT32_C(0xc0000000)
#define KM_MIPS32_KSSEG_SIZE	UINT32_C(0x20000000)

#define KM_MIPS32_KSEG3_START	UINT32_C(0xe0000000)
#define KM_MIPS32_KSEG3_SIZE	UINT32_C(0x20000000)

extern void km_identity_arch_init(void);
extern void km_non_identity_arch_init(void);
extern bool km_is_non_identity_arch(uintptr_t);

#endif

/** @}
 */
