/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_sparc64_ASM_H_
#define BOOT_sparc64_ASM_H_

#include <stdint.h>

extern void jump_to_kernel(uintptr_t physmem_start, bootinfo_t *bootinfo,
    uint8_t subarch, void *entry) __attribute__((noreturn));

#endif
