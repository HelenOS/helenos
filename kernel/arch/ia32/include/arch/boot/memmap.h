/*
 * Copyright (c) 2005 Josef Cejka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup ia32
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
