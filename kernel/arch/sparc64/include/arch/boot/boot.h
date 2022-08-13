/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_BOOT_H_
#define KERN_sparc64_BOOT_H_

#define VMA  0x400000
#define LMA  VMA

#ifndef __ASSEMBLER__
#ifndef __LINKER__

#include <config.h>
#include <typedefs.h>
#include <genarch/ofw/ofw_tree.h>

#define TASKMAP_MAX_RECORDS  32
#define MEMMAP_MAX_RECORDS   32

#define BOOTINFO_TASK_NAME_BUFLEN  32

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

/** Bootinfo structure.
 *
 * Must be in sync with bootinfo structure used by the boot loader.
 *
 */
typedef struct {
	uintptr_t physmem_start;
	taskmap_t taskmap;
	memmap_t memmap;
	ballocs_t ballocs;
	ofw_tree_node_t *ofw_root;
} bootinfo_t;

extern memmap_t memmap;

#endif
#endif

#endif

/** @}
 */
