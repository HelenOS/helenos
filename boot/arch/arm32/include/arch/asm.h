/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm32
 * @{
 */
/** @file
 * @brief Functions implemented in assembly.
 */

#ifndef BOOT_arm32_ASM_H
#define BOOT_arm32_ASM_H

#include <arch/arch.h>
#include <arch/mm.h>

extern pte_level0_section_t boot_pt[PTL0_ENTRIES];
extern void *boot_stack;

/** Jump to the kernel entry point.
 *
 * @param entry    Kernel entry point.
 * @param bootinfo Structure holding information about loaded tasks.
 *
 */
extern void jump_to_kernel(void *entry, void *bootinfo)
    __attribute__((noreturn));

#endif

/** @}
 */
