/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Boot related declarations.
 */

#ifndef BOOT_arm64_MAIN_H
#define BOOT_arm64_MAIN_H

#include <genarch/efi.h>

extern efi_status_t bootstrap(void *efi_handle_in,
    efi_system_table_t *efi_system_table_in, void *load_address);

#endif

/** @}
 */
