/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_riscv64_TYPES_H_
#define BOOT_riscv64_TYPES_H_

#include <_bits/all.h>

#define MEMMAP_MAX_RECORDS         32
#define TASKMAP_MAX_RECORDS        32
#define BOOTINFO_TASK_NAME_BUFLEN  32

typedef struct {
	volatile uint64_t *tohost;
	volatile uint64_t *fromhost;
} ucbinfo_t;

typedef struct {
	void *start;
	size_t size;
} memzone_t;

typedef struct {
	uint64_t total;
	size_t cnt;
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

typedef struct {
	/** Address where the task was placed. */
	void *addr;
	/** Size of the task's binary. */
	size_t size;
	/** Task name. */
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} task_t;

typedef struct {
	size_t cnt;
	task_t tasks[TASKMAP_MAX_RECORDS];
} taskmap_t;

typedef struct {
	ucbinfo_t ucbinfo;
	uintptr_t physmem_start;
	uintptr_t htif_frame;
	uintptr_t pt_frame;
	memmap_t memmap;
	taskmap_t taskmap;
} bootinfo_t;

#endif
