/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_mips32_TYPES_H_
#define BOOT_mips32_TYPES_H_

#include <_bits/all.h>

#define TASKMAP_MAX_RECORDS        32
#define CPUMAP_MAX_RECORDS         32
#define BOOTINFO_TASK_NAME_BUFLEN  32
#define BOOTINFO_BOOTARGS_BUFLEN   256

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
#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	uint32_t sdram_size;
#endif
	uint32_t cpumap;
	taskmap_t taskmap;
	char bootargs[BOOTINFO_BOOTARGS_BUFLEN];
} bootinfo_t;

#endif
