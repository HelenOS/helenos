/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Various ARM64-specific macros.
 */

#ifndef BOOT_arm64_ARCH_H
#define BOOT_arm64_ARCH_H

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define BOOT_OFFSET  0x80000

#ifndef __ASSEMBLER__
#define KA2PA(x)  (((uintptr_t) (x)) - UINT64_C(0xffffffff00000000))
#endif

#endif

/** @}
 */
