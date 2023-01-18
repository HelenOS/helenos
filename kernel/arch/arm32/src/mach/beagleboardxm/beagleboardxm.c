/*
 * Copyright (c) 2012 Jan Vesely
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
/** @addtogroup kernel_arm32_beagleboardxm
 * @{
 */
/** @file
 *  @brief BeagleBoard-xM platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/beagleboardxm/beagleboardxm.h>
#include <assert.h>
#include <genarch/drivers/amdm37x/uart.h>
#include <genarch/drivers/amdm37x/irc.h>
#include <genarch/drivers/amdm37x/gpt.h>
#include <genarch/fb/fb.h>
#include <genarch/srln/srln.h>
#include <interrupt.h>
#include <mm/km.h>
#include <ddi/ddi.h>
#include <stdbool.h>

static void bbxm_init(void);
static void bbxm_timer_irq_start(void);
static void bbxm_cpu_halt(void);
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size);
static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate);
static void bbxm_frame_init(void);
static void bbxm_output_init(void);
static void bbxm_input_init(void);
static size_t bbxm_get_irq_count(void);
static const char *bbxm_get_platform_name(void);

#define BBXM_MEMORY_START	0x80000000	/* physical */
#define BBXM_MEMORY_SIZE	0x20000000	/* 512 MB */

static struct beagleboard {
	omap_irc_regs_t *irc_addr;
	omap_uart_t uart;
	amdm37x_gpt_t timer;
} beagleboard;

struct arm_machine_ops bbxm_machine_ops = {
	.machine_init = bbxm_init,
	.machine_timer_irq_start = bbxm_timer_irq_start,
	.machine_cpu_halt = bbxm_cpu_halt,
	.machine_get_memory_extents = bbxm_get_memory_extents,
	.machine_irq_exception = bbxm_irq_exception,
	.machine_frame_init = bbxm_frame_init,
	.machine_output_init = bbxm_output_init,
	.machine_input_init = bbxm_input_init,
	.machine_get_irq_count = bbxm_get_irq_count,
	.machine_get_platform_name = bbxm_get_platform_name
};

static irq_ownership_t bb_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void bb_timer_irq_handler(irq_t *irq)
{
	amdm37x_gpt_irq_ack(&beagleboard.timer);

	/*
	 * We are holding a lock which prevents preemption.
	 * Release the lock, call clock() and reacquire the lock again.
	 */
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);
}

static void bbxm_init(void)
{
	/* Initialize interrupt controller */
	beagleboard.irc_addr =
	    (void *) km_map(AMDM37x_IRC_BASE_ADDRESS, AMDM37x_IRC_SIZE,
	    KM_NATURAL_ALIGNMENT, PAGE_NOT_CACHEABLE);
	assert(beagleboard.irc_addr);
	omap_irc_init(beagleboard.irc_addr);

	/*
	 * Initialize timer. Use timer1, because it is in WKUP power domain
	 * (always on) and has special capabilities for precise 1ms ticks
	 */
	amdm37x_gpt_timer_ticks_init(&beagleboard.timer,
	    AMDM37x_GPT1_BASE_ADDRESS, AMDM37x_GPT1_SIZE, HZ);
}

static void bbxm_timer_irq_start(void)
{
	/* Initialize timer IRQ */
	static irq_t timer_irq;
	irq_initialize(&timer_irq);
	timer_irq.inr = AMDM37x_GPT1_IRQ;
	timer_irq.claim = bb_timer_irq_claim;
	timer_irq.handler = bb_timer_irq_handler;
	irq_register(&timer_irq);

	/* Enable timer interrupt */
	omap_irc_enable(beagleboard.irc_addr, AMDM37x_GPT1_IRQ);

	/* Start timer here */
	amdm37x_gpt_timer_ticks_start(&beagleboard.timer);
}

static void bbxm_cpu_halt(void)
{
	while (true)
		;
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = BBXM_MEMORY_START;
	*size = BBXM_MEMORY_SIZE;
}

static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate)
{
	const unsigned inum = omap_irc_inum_get(beagleboard.irc_addr);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
		/* Spurious interrupt. */
		printf("cpu%d: spurious interrupt (inum=%d)\n",
		    CPU->id, inum);
	}
	/** amdm37x manual ch. 12.5.2 (p. 2428) places irc ack at the end
	 * of ISR. DO this to avoid strange behavior.
	 */
	omap_irc_irq_ack(beagleboard.irc_addr);
}

static void bbxm_frame_init(void)
{
}

static void bbxm_output_init(void)
{
#ifdef CONFIG_OMAP_UART
	/* UART3 is wired to external RS232 connector */
	const bool ok = omap_uart_init(&beagleboard.uart,
	    AMDM37x_UART3_IRQ, AMDM37x_UART3_BASE_ADDRESS, AMDM37x_UART3_SIZE);
	if (ok) {
		stdout_wire(&beagleboard.uart.outdev);
	}
#endif
}

static void bbxm_input_init(void)
{
#ifdef CONFIG_OMAP_UART
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);
		omap_uart_input_wire(&beagleboard.uart, srln);
		omap_irc_enable(beagleboard.irc_addr, AMDM37x_UART3_IRQ);
	}
#endif
}

size_t bbxm_get_irq_count(void)
{
	return AMDM37x_IRC_IRQ_COUNT;
}

const char *bbxm_get_platform_name(void)
{
	return "beagleboardxm";
}

/**
 * @}
 */
