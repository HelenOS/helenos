/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Various ARM64-specific macros.
 */

#ifndef KERN_arm64_ARCH_H_
#define KERN_arm64_ARCH_H_

#include <arch/boot/boot.h>

extern void arm64_pre_main(void *entry, bootinfo_t *bootinfo);

#endif

/** @}
 */
