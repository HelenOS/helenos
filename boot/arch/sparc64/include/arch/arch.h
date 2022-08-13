/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_sparc64_ARCH_H_
#define BOOT_sparc64_ARCH_H_

#define PAGE_WIDTH  14
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define LOADER_ADDRESS  0x004000
#define KERNEL_ADDRESS  0x400000

#define STACK_SIZE                   8192
#define STACK_ALIGNMENT              16
#define STACK_BIAS                   2047
#define STACK_WINDOW_SAVE_AREA_SIZE  (16 * 8)
#define STACK_ARG_SAVE_AREA_SIZE     (6 * 8)

#define NWINDOWS  8

#define PSTATE_IE_BIT    2
#define PSTATE_PRIV_BIT  4
#define PSTATE_AM_BIT    8

#define ASI_ICBUS_CONFIG        0x4a
#define ICBUS_CONFIG_MID_SHIFT  17

/** Constants to distinguish particular UltraSPARC architecture */
#define ARCH_SUN4U  10
#define ARCH_SUN4V  20

/** Constants to distinguish particular UltraSPARC subarchitecture */
#define SUBARCH_UNKNOWN  0
#define SUBARCH_US       1
#define SUBARCH_US3      3

#define BSP_PROCESSOR  1
#define AP_PROCESSOR   0

#endif
