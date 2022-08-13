/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_ARCH_H_
#define KERN_mips32_ARCH_H_

#include <stddef.h>
#include <stdint.h>

#define TASKMAP_MAX_RECORDS        32
#define CPUMAP_MAX_RECORDS         32
#define BOOTINFO_TASK_NAME_BUFLEN  32
#define BOOTINFO_BOOTARGS_BUFLEN   256

extern size_t cpu_count;

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
extern size_t sdram_size;
#endif

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
#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	uint32_t sdram_size;
#endif
	uint32_t cpumap;
	taskmap_t taskmap;
	char bootargs[BOOTINFO_BOOTARGS_BUFLEN];
} bootinfo_t;

extern void mips32_pre_main(void *entry, bootinfo_t *bootinfo);

#endif

/** @}
 */
