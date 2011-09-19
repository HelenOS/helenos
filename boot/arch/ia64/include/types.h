/*
 * Copyright (c) 2005 Martin Decky
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

#ifndef BOOT_ia64_TYPES_H_
#define BOOT_ia64_TYPES_H_

#include <arch/common.h>

#define TASKMAP_MAX_RECORDS		32
#define BOOTINFO_TASK_NAME_BUFLEN	32
#define MEMMAP_ITEMS			128

typedef uint64_t size_t;
typedef uint64_t sysarg_t;
typedef uint64_t uintptr_t;

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} binit_task_t;

typedef struct {
	size_t cnt;
	binit_task_t tasks[TASKMAP_MAX_RECORDS];
} binit_t;

typedef struct {
	unsigned int type;
	unsigned long base;
	unsigned long size;
} memmap_item_t;

typedef struct {
	binit_t taskmap;

	memmap_item_t memmap[MEMMAP_ITEMS];
	unsigned int memmap_items;

	sysarg_t *sapic;
	unsigned long sys_freq;
	unsigned long freq_scale;
	unsigned int wakeup_intno;
} bootinfo_t;

/** This is a minimal ELILO-compatible boot parameter structure. */
typedef struct {
	uint64_t cmd_line;
	uint64_t efi_system_table;
	uint64_t efi_memmap;
	uint64_t efi_memmap_sz;
	uint64_t efi_memdesc_sz;
} boot_param_t;

#endif
