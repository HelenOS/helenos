/*
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32boot
 * @{
 */
/** @file
 * @brief Memory management used while booting the kernel.
 *
 * So called "section" paging is used while booting the kernel. The term
 * "section" comes from the ARM architecture specification and stands for the
 * following: one-level paging, 1MB sized pages, 4096 entries in the page
 * table.
 */

#ifndef BOOT_sparc32_MM_H
#define BOOT_sparc32_MM_H

#include <typedefs.h>

#define PAGE_SIZE  (1 << 12)

typedef struct {
	uint32_t pa;
	uint32_t size;
	uint32_t va;
	uint32_t cacheable;
} section_mapping_t;

typedef struct {
	unsigned int ppn : 24;
	unsigned int cacheable : 1;
	unsigned int modified : 1;
	unsigned int referenced : 1;
	unsigned int acc : 3;
	unsigned int et : 2;
} __attribute__((packed)) pte_t;

extern pte_t boot_pt[PTL0_ENTRIES];

extern void mmu_init(void);

#define PTE_ET_DESCRIPTOR  1
#define PTE_ET_ENTRY       2
#define PTE_ACC_RWX        3
#define MMU_CONTROL_EN     (1 << 0)

#endif

/** @}
 */
