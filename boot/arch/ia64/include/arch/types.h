/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ia64_TYPES_H_
#define BOOT_ia64_TYPES_H_

#include <_bits/all.h>

#define TASKMAP_MAX_RECORDS		32
#define BOOTINFO_TASK_NAME_BUFLEN	32
#define MEMMAP_ITEMS			128

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} task_t;

typedef struct {
	size_t cnt;
	task_t tasks[TASKMAP_MAX_RECORDS];
} taskmap_t;

typedef struct {
	unsigned int type;
	unsigned long base;
	unsigned long size;
} memmap_item_t;

typedef struct {
	taskmap_t taskmap;

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
