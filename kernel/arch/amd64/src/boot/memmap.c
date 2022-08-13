/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#include <arch/boot/memmap.h>

uint8_t e820counter = 0;
e820memmap_t e820table[MEMMAP_E820_MAX_RECORDS];

/** @}
 */
