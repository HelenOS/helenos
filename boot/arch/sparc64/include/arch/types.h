/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_sparc64_TYPES_H_
#define BOOT_sparc64_TYPES_H_

#include <_bits/all.h>

#define TASKMAP_MAX_RECORDS        32
#define BOOTINFO_TASK_NAME_BUFLEN  32

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} task_t;

typedef struct {
	size_t cnt;
	task_t tasks[TASKMAP_MAX_RECORDS];
} taskmap_t;

#endif
