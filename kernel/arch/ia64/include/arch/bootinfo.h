/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_ia64_BOOTINFO_H_
#define KERN_ia64_BOOTINFO_H_

#define TASKMAP_MAX_RECORDS  32

#define MEMMAP_ITEMS 128

#define MEMMAP_FREE_MEM 0

/** Size of buffer for storing task name in utask_t. */
#define BOOTINFO_TASK_NAME_BUFLEN 32

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} utask_t;

typedef struct {
	size_t cnt;
	utask_t tasks[TASKMAP_MAX_RECORDS];
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

extern bootinfo_t *bootinfo;

#endif
