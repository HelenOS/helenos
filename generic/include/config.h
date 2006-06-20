/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <arch/types.h>
#include <typedefs.h>
#include <arch/mm/page.h>

#define STACK_SIZE		PAGE_SIZE

#define CONFIG_MEMORY_SIZE	(8 * 1024 * 1024)

#define CONFIG_INIT_TASKS	32

typedef struct {
	__address addr;
	size_t size;
} init_task_t;

typedef struct {
	count_t cnt;
	init_task_t tasks[CONFIG_INIT_TASKS];
} init_t;

typedef struct {
	count_t cpu_count;		/**< Number of processors detected. */
	volatile count_t cpu_active;	/**< Number of processors that are up and running. */

	__address base;
	size_t memory_size;		/**< Size of detected memory in bytes. */
	size_t kernel_size;		/**< Size of memory in bytes taken by kernel and stack */
} config_t;

extern config_t config;
extern init_t init;

#endif

/** @}
 */
