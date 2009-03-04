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

#ifndef BOOT_mips32_MAIN_H_
#define BOOT_mips32_MAIN_H_

#define CPUMAP               0x80001000
#define INITIAL_STACK        0x80002000
#define MSIM_DORDER_ADDRESS  0xb0000004

#define TASKMAP_MAX_RECORDS  32
#define CPUMAP_MAX_RECORDS   32

#ifndef __ASM__

/** Size of buffer for storing task name in task_t. */
#define BOOTINFO_TASK_NAME_BUFLEN 32

/** Struct holding information about single loaded task. */
typedef struct {
	/** Address where the task was placed. */
	void *addr;
	/** Size of the task's binary. */
	unsigned int size;
	/** Task name. */
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} task_t;

typedef struct {
	unsigned int cpumap;
	unsigned int cnt;
	task_t tasks[TASKMAP_MAX_RECORDS];
} bootinfo_t;

extern void start(void);
extern void bootstrap(void);

#endif

#endif
