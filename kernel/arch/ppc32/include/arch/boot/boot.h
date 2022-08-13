/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_BOOT_H_
#define KERN_ppc32_BOOT_H_

#define BOOT_OFFSET  0x8000

#define TASKMAP_MAX_RECORDS        32
#define MEMMAP_MAX_RECORDS         32
#define BOOTINFO_TASK_NAME_BUFLEN  32

#ifndef __ASSEMBLER__

#include <config.h>
#include <genarch/ofw/ofw_tree.h>
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
	void *start;
	size_t size;
} memzone_t;

typedef struct {
	uint64_t total;
	size_t cnt;
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

typedef struct {
	memmap_t memmap;
	taskmap_t taskmap;
	ballocs_t ballocs;
	ofw_tree_node_t *ofw_root;
} bootinfo_t;

extern memmap_t memmap;

#endif

#endif

/** @}
 */
