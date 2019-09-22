/*
 * Copyright (c) 2015 Petr Pavlu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Shared interface between the bootcode and the kernel.
 */

#ifndef KERN_arm64_BOOT_H_
#define KERN_arm64_BOOT_H_

#define BOOT_OFFSET  0x80000

#ifndef __ASSEMBLER__
#include <stddef.h>

#define BOOTINFO_TASK_NAME_BUFLEN  32
#define TASKMAP_MAX_RECORDS        32
#define MEMMAP_MAX_RECORDS        128

/** Task structure. */
typedef struct {
	/** Address where the task was placed. */
	void *addr;
	/** Size of the task's binary. */
	size_t size;
	/** Task name. */
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} utask_t;

/** Task map structure. */
typedef struct {
	/** Number of boot tasks. */
	size_t cnt;
	/** Boot task data. */
	utask_t tasks[TASKMAP_MAX_RECORDS];
} taskmap_t;

/** Memory zone types. */
typedef enum {
	/** Unusuable memory. */
	MEMTYPE_UNUSABLE,
	/** Usable memory. */
	MEMTYPE_AVAILABLE,
	/** Memory that can be used after ACPI is enabled. */
	MEMTYPE_ACPI_RECLAIM
} memtype_t;

/** Memory area. */
typedef struct {
	/** Type of the memory. */
	memtype_t type;
	/** Address of the area. */
	void *start;
	/** Size of the area. */
	size_t size;
} memzone_t;

/** System memory map. */
typedef struct {
	/** Number of memory zones. */
	size_t cnt;
	/** Memory zones. */
	memzone_t zones[MEMMAP_MAX_RECORDS];
} memmap_t;

/** Bootinfo structure. */
typedef struct {
	/** Task map. */
	taskmap_t taskmap;
	/** Memory map. */
	memmap_t memmap;
} bootinfo_t;

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
