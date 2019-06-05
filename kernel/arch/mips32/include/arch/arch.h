/*
 * Copyright (c) 2005 Martin Decky
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
