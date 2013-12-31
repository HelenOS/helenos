/*
 * Copyright (c) 2010 Martin Decky
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup sparc32
 * @{
 */
/** @file
 */

#ifndef KERN_sparc32_ARCH_H_
#define KERN_sparc32_ARCH_H_

#ifndef __ASM__

#include <typedefs.h>
#include <arch/istate.h>

#define NWINDOWS  8

/* ASI assignments: */
#define ASI_CACHEMISS  0x01
#define ASI_CACHECTRL  0x02
#define ASI_MMUCACHE   0x10
#define ASI_MMUREGS    0x19
#define ASI_MMUBYPASS  0x1c
#define ASI_MMUFLUSH   0x18

#define TASKMAP_MAX_RECORDS  32
#define CPUMAP_MAX_RECORDS   32

#define BOOTINFO_TASK_NAME_BUFLEN  32

typedef struct {
	void *addr;
	size_t size;
	char name[BOOTINFO_TASK_NAME_BUFLEN];
} utask_t;

typedef struct {
	size_t cnt;
	utask_t tasks[TASKMAP_MAX_RECORDS];
	/* Fields below are LEON-specific */
	uintptr_t uart_base;
	uintptr_t intc_base;
	uintptr_t timer_base;
	int uart_irq;
	int timer_irq;
	uint32_t memsize;
} bootinfo_t;

extern void arch_pre_main(void *, bootinfo_t *);
extern void write_to_invalid(uint32_t, uint32_t, uint32_t);
extern void read_from_invalid(uint32_t *, uint32_t *, uint32_t *);
extern void preemptible_save_uspace(uintptr_t, istate_t *);
extern void preemptible_restore_uspace(uintptr_t, istate_t *);
extern void flush_windows(void);

#endif

#endif

/** @}
 */
