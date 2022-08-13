/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_CONFIG_H_
#define KERN_CONFIG_H_

#include <arch/mm/page.h>
#include <macros.h>

#define STACK_FRAMES  2
#define STACK_SIZE    FRAMES2SIZE(STACK_FRAMES)

#define STACK_SIZE_USER  (1 * 1024 * 1024)

#define CONFIG_BOOT_ARGUMENTS_BUFLEN 256

#define CONFIG_INIT_TASKS        32
#define CONFIG_TASK_NAME_BUFLEN  32
#define CONFIG_TASK_ARGUMENTS_BUFLEN 64

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <typedefs.h>

typedef struct {
	uintptr_t paddr;
	size_t size;
	char name[CONFIG_TASK_NAME_BUFLEN];
	char arguments[CONFIG_TASK_ARGUMENTS_BUFLEN];
} init_task_t;

typedef struct {
	size_t cnt;
	init_task_t tasks[CONFIG_INIT_TASKS];
} init_t;

/** Boot allocations.
 *
 * Allocatations made by the boot that are meant to be used by the kernel
 * are all recorded in the ballocs_t type.
 */
typedef struct {
	uintptr_t base;
	size_t size;
} ballocs_t;

typedef struct {
	/** Number of processors detected. */
	unsigned int cpu_count;
	/** Number of processors that are up and running. */
	volatile size_t cpu_active;

	uintptr_t base;
	/** Size of memory in bytes taken by kernel and stack. */
	size_t kernel_size;

	/** Base adddress of initial stack. */
	uintptr_t stack_base;
	/** Size of initial stack. */
	size_t stack_size;

	bool identity_configured;
	/** Base address of the kernel identity mapped memory. */
	uintptr_t identity_base;
	/** Size of the kernel identity mapped memory. */
	size_t identity_size;

	bool non_identity_configured;

	/** End of physical memory. */
	uint64_t physmem_end;
} config_t;

extern config_t config;
extern char bargs[];
extern init_t init;
extern ballocs_t ballocs;

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
