/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ppc32_ASM_H_
#define BOOT_ppc32_ASM_H_

#include <stddef.h>
#include <arch/main.h>

extern void jump_to_kernel(void *, void *, size_t, void *, uintptr_t)
    __attribute__((noreturn));
extern void real_mode(void);

#endif
