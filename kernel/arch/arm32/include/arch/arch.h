/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Empty.
 */

#ifndef KERN_arm32_ARCH_H_
#define KERN_arm32_ARCH_H_

#define TASKMAP_MAX_RECORDS  32
#define CPUMAP_MAX_RECORDS   32

#define BOOTINFO_TASK_NAME_BUFLEN 32

#include <stddef.h>

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
	taskmap_t taskmap;
} bootinfo_t;

extern void arm32_pre_main(void *entry, bootinfo_t *bootinfo);

#endif

/** @}
 */
