/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_riscv64_ASM_H_
#define BOOT_riscv64_ASM_H_

#include <stddef.h>
#include <stdint.h>

extern char htif_page[];
extern char pt_page[];

extern _Noreturn void jump_to_kernel(uintptr_t, uintptr_t);

#endif
