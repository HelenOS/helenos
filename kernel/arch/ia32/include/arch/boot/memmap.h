/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_MEMMAP_H_
#define KERN_ia32_MEMMAP_H_

#include <arch/boot/memmap_struct.h>

/* E820h memory range types */

/* Free memory */
#define MEMMAP_MEMORY_AVAILABLE  1

/* Not available for OS */
#define MEMMAP_MEMORY_RESERVED   2

/* OS may use it after reading ACPI table */
#define MEMMAP_MEMORY_ACPI       3

/* Unusable, required to be saved and restored across an NVS sleep */
#define MEMMAP_MEMORY_NVS        4

/* Corrupted memory */
#define MEMMAP_MEMORY_UNUSABLE   5

/* Size of one entry */
#define MEMMAP_E820_RECORD_SIZE  20

/* Maximum entries */
#define MEMMAP_E820_MAX_RECORDS  32

#ifndef __ASSEMBLER__

#include <stdint.h>

extern e820memmap_t e820table[MEMMAP_E820_MAX_RECORDS];
extern uint8_t e820counter;

#endif

#endif

/** @}
 */
