/*
 * SPDX-FileCopyrightText: 2019 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Image self-relocation support.
 */

#ifndef BOOT_arm64_RELOCATE_H
#define BOOT_arm64_RELOCATE_H

#include <abi/elf.h>
#include <genarch/efi.h>
#include <stdint.h>

extern efi_status_t self_relocate(uintptr_t base, const elf_dyn_t *dyn);

#endif

/** @}
 */
