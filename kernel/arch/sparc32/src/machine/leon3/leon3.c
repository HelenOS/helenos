/*
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

#include <arch.h>
#include <typedefs.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/machine_func.h>
#include <arch/machine/leon3/leon3.h>
#include <genarch/drivers/grlib/uart.h>
#include <genarch/drivers/grlib/irqmp.h>
#include <genarch/srln/srln.h>
#include <func.h>
#include <config.h>
#include <errno.h>
#include <context.h>
#include <fpu_context.h>
#include <interrupt.h>
#include <syscall/copy.h>
#include <ddi/irq.h>
#include <proc/thread.h>
#include <syscall/syscall.h>
#include <console/console.h>
#include <macros.h>
#include <memstr.h>
#include <str.h>

static void leon3_init(bootinfo_t *);
static void leon3_cpu_halt(void);
static void leon3_get_memory_extents(uintptr_t *, size_t *);
static void leon3_timer_start(void);
static void leon3_irq_exception(unsigned int, istate_t *);
static void leon3_output_init(void);
static void leon3_input_init(void);
static size_t leon3_get_irq_count(void);
static const char *leon3_get_platform_name(void);

struct leon3_machine_t {
	bootinfo_t *bootinfo;
	outdev_t *scons_dev;
	grlib_irqmp_t irqmp;
};

struct sparc_machine_ops leon3_machine_ops = {
	.machine_init = leon3_init,
	.machine_cpu_halt = leon3_cpu_halt,
	.machine_get_memory_extents = leon3_get_memory_extents,
	.machine_timer_irq_start = leon3_timer_start,
	.machine_irq_exception = leon3_irq_exception,
	.machine_output_init = leon3_output_init,
	.machine_input_init = leon3_input_init,
	.machine_get_irq_count = leon3_get_irq_count,
	.machine_get_platform_name = leon3_get_platform_name
};

static struct leon3_machine_t machine;

static void leon3_init(bootinfo_t *bootinfo)
{
	machine.bootinfo = bootinfo;
	grlib_irqmp_init(&machine.irqmp, bootinfo);
}

static void leon3_cpu_halt(void)
{
	// FIXME TODO
	while (1);
}

static void leon3_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = LEON3_SDRAM_START;
	*size = 64 * 1024 * 1024;
	// FIXME: *size = machine.bootinfo->memsize;
}

static void leon3_timer_start(void)
{
	// FIXME:
	// machine.timer =
	//     grlib_timer_init(machine.bootinfo->timer_base,
	//     machine.bootinfo->timer_irq);
}

static void leon3_irq_exception(unsigned int exc, istate_t *istate)
{
	int irqnum = grlib_irqmp_inum_get(&machine.irqmp);
	
	grlib_irqmp_clear(&machine.irqmp, irqnum);
	
	irq_t *irq = irq_dispatch_and_lock(exc);
	if (irq) {
		irq->handler(irq);
		spinlock_unlock(&irq->lock);
	} else
		printf("cpu%d: spurious interrupt (irq=%d)\n", CPU->id, irqnum);
}

static void leon3_output_init(void)
{
	machine.scons_dev =
	    grlib_uart_init(machine.bootinfo->uart_base,
	    machine.bootinfo->uart_irq);
	
	if (machine.scons_dev)
		stdout_wire(machine.scons_dev);
}

static void leon3_input_init(void)
{
	grlib_uart_t *scons_inst;
	
	if (machine.scons_dev) {
		/* Create input device. */
		scons_inst = (void *)machine.scons_dev->data;
		
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			grlib_uart_input_wire(scons_inst, srln);
			
			/* Enable interrupts from UART */
			grlib_irqmp_unmask(&machine.irqmp,
			    machine.bootinfo->uart_irq);
		}
	}
}

static size_t leon3_get_irq_count(void)
{
	return LEON3_IRQ_COUNT;
}

static const char *leon3_get_platform_name(void)
{
	return "LEON3";
}
