/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ia64_ARCH_H_
#define BOOT_ia64_ARCH_H_

#define PAGE_WIDTH  14
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define LOADER_ADDRESS  0x4400000
#define KERNEL_ADDRESS  0x4800000
#define KERNEL_VADDRESS 0xe000000004800000

#define STACK_SIZE                   8192
#define STACK_ALIGNMENT              16

#endif
