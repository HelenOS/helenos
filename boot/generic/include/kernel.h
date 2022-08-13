/*
 * SPDX-FileCopyrightText: 2018 Jiří Zárevúcky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ELF_H_
#define BOOT_ELF_H_

#include <stdint.h>

uintptr_t check_kernel_translated(void *, uintptr_t);
uintptr_t check_kernel(void *);

#endif
