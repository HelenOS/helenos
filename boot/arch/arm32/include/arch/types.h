/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm32
 * @{
 */
/** @file
 * @brief Definitions of basic types like #uintptr_t.
 */

#ifndef BOOT_arm32_TYPES_H
#define BOOT_arm32_TYPES_H

#include <_bits/all.h>

#define TASKMAP_MAX_RECORDS        32
#define BOOTINFO_TASK_NAME_BUFLEN  32

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
	taskmap_t taskmap;
} bootinfo_t;

#endif

/** @}
 */
