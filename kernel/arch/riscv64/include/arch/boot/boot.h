/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_BOOT_H_
#define KERN_riscv64_BOOT_H_

#define BOOT_OFFSET  0x48000000

#define TASKMAP_MAX_RECORDS        32
#define MEMMAP_MAX_RECORDS         32
#define BOOTINFO_TASK_NAME_BUFLEN  32

/* Temporary stack size for boot process */
#define TEMP_STACK_SIZE  0x1000

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>
#include <config.h>

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
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} utask_t;

typedef struct {
	size_t cnt;
	utask_t tasks[TASKMAP_MAX_RECORDS];
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

#endif

/** @}
 */
