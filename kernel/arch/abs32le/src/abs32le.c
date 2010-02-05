/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup abs32le
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/types.h>
#include <arch/context.h>
#include <arch/interrupt.h>
#include <arch/asm.h>

#include <config.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <memstr.h>

void arch_pre_mm_init(void)
{
}

void arch_post_mm_init(void)
{
	if (config.cpu_active == 1) {
		/* Initialize IRQ routing */
		irq_init(0, 0);
		
		/* Merge all memory zones to 1 big zone */
		zone_merge_all();
	}
}

void arch_post_cpu_init()
{
}

void arch_pre_smp_init(void)
{
}

void arch_post_smp_init(void)
{
}

void calibrate_delay_loop(void)
{
}

unative_t sys_tls_set(unative_t addr)
{
	return 0;
}

/** Construct function pointer
 *
 * @param fptr   function pointer structure
 * @param addr   function address
 * @param caller calling function address
 *
 * @return address of the function pointer
 *
 */
void *arch_construct_function(fncptr_t *fptr, void *addr, void *caller)
{
	return addr;
}

void arch_reboot(void)
{
}

void irq_initialize_arch(irq_t *irq)
{
	(void) irq;
}

void memsetb(void *dst, size_t cnt, uint8_t val)
{
	_memsetb(dst, cnt, val);
}

void memsetw(void *dst, size_t cnt, uint16_t val)
{
	_memsetw(dst, cnt, val);
}

void panic_printf(char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	
	halt();
}

/** @}
 */
