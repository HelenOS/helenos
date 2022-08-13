/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Functions implemented in assembly.
 */

#ifndef BOOT_arm64_ASM_H
#define BOOT_arm64_ASM_H

/** Jump to the kernel entry point.
 *
 * @param entry    Kernel entry point.
 * @param bootinfo Structure holding information about loaded tasks.
 */
extern void jump_to_kernel(void *entry, void *bootinfo)
    __attribute__((noreturn));

#endif

/** @}
 */
