/*
 * Copyright (c) 2012 Matteo Facchinetti
 * Copyright (c) 2012 Maurizio Lombardi
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
/** @addtogroup arm32beaglebone
 * @{
 */
/** @file
 *  @brief BeagleBone platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/beaglebone/beaglebone.h>
#include <assert.h>
#include <genarch/drivers/am335x/irc.h>
#include <genarch/drivers/am335x/uart.h>
#include <genarch/drivers/am335x/timer.h>
#include <genarch/drivers/am335x/cm_per.h>
#include <genarch/drivers/am335x/cm_dpll.h>
#include <genarch/drivers/am335x/ctrl_module.h>
#include <genarch/srln/srln.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <mm/km.h>
#include <stdbool.h>

#define BBONE_MEMORY_START       0x80000000      /* physical */
#define BBONE_MEMORY_SIZE        0x10000000      /* 256 MB */

static void bbone_init(void);
static void bbone_timer_irq_start(void);
static void bbone_cpu_halt(void);
static void bbone_get_memory_extents(uintptr_t *start, size_t *size);
static void bbone_irq_exception(unsigned int exc_no, istate_t *istate);
static void bbone_frame_init(void);
static void bbone_output_init(void);
static void bbone_input_init(void);
static size_t bbone_get_irq_count(void);
static const char *bbone_get_platform_name(void);

static struct beaglebone {
	omap_irc_regs_t *irc_addr;
	am335x_cm_per_regs_t *cm_per_addr;
	am335x_cm_dpll_regs_t *cm_dpll_addr;
	am335x_ctrl_module_t  *ctrl_module;
	am335x_timer_t timer;
	omap_uart_t uart;
} bbone;

struct arm_machine_ops bbone_machine_ops = {
	.machine_init = bbone_init,
	.machine_timer_irq_start = bbone_timer_irq_start,
	.machine_cpu_halt = bbone_cpu_halt,
	.machine_get_memory_extents = bbone_get_memory_extents,
	.machine_irq_exception = bbone_irq_exception,
	.machine_frame_init = bbone_frame_init,
	.machine_output_init = bbone_output_init,
	.machine_input_init = bbone_input_init,
	.machine_get_irq_count = bbone_get_irq_count,
	.machine_get_platform_name = bbone_get_platform_name,
};

static void bbone_init(void)
{
	bbone.irc_addr = (void *) km_map(AM335x_IRC_BASE_ADDRESS,
	    AM335x_IRC_SIZE, PAGE_NOT_CACHEABLE);

	bbone.cm_per_addr = (void *) km_map(AM335x_CM_PER_BASE_ADDRESS,
	    AM335x_CM_PER_SIZE, PAGE_NOT_CACHEABLE);

	bbone.cm_dpll_addr = (void *) km_map(AM335x_CM_DPLL_BASE_ADDRESS,
	    AM335x_CM_DPLL_SIZE, PAGE_NOT_CACHEABLE);

	bbone.ctrl_module = (void *) km_map(AM335x_CTRL_MODULE_BASE_ADDRESS,
	    AM335x_CTRL_MODULE_SIZE, PAGE_NOT_CACHEABLE);

	assert(bbone.irc_addr != NULL);
	assert(bbone.cm_per_addr != NULL);
	assert(bbone.cm_dpll_addr != NULL);
	assert(bbone.ctrl_module != NULL);

	/* Initialize the interrupt controller */
	omap_irc_init(bbone.irc_addr);
}

static irq_ownership_t bbone_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void bbone_timer_irq_handler(irq_t *irq)
{
	am335x_timer_intr_ack(&bbone.timer);
	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);
}

static void bbone_timer_irq_start(void)
{
	unsigned sysclk_freq;
	errno_t rc;

	/* Initialize the IRQ */
	static irq_t timer_irq;
	irq_initialize(&timer_irq);
	timer_irq.inr = AM335x_DMTIMER2_IRQ;
	timer_irq.claim = bbone_timer_irq_claim;
	timer_irq.handler = bbone_timer_irq_handler;
	irq_register(&timer_irq);

	/* Enable the DMTIMER2 clock module */
	am335x_clock_module_enable(bbone.cm_per_addr, DMTIMER2);
	/* Select the SYSCLK as the clock source for the dmtimer2 module */
	am335x_clock_source_select(bbone.cm_dpll_addr, DMTIMER2,
	    CLK_SRC_M_OSC);
	/* Initialize the DMTIMER2 */
	if (am335x_ctrl_module_clock_freq_get(bbone.ctrl_module,
	    &sysclk_freq) != EOK) {
		printf("Cannot get the system clock frequency!\n");
		return;
	} else
		printf("system clock running at %u hz\n", sysclk_freq);

	rc = am335x_timer_init(&bbone.timer, DMTIMER2, HZ, sysclk_freq);
	if (rc != EOK) {
		printf("Timer initialization failed\n");
		return;
	}
	/* Enable the interrupt */
	omap_irc_enable(bbone.irc_addr, AM335x_DMTIMER2_IRQ);
	/* Start the timer */
	am335x_timer_start(&bbone.timer);
}

static void bbone_cpu_halt(void)
{
	while (true)
		;
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbone_get_memory_extents(uintptr_t *start, size_t *size)
{
	*start = BBONE_MEMORY_START;
	*size  = BBONE_MEMORY_SIZE;
}

static void bbone_irq_exception(unsigned int exc_no, istate_t *istate)
{
	const unsigned inum = omap_irc_inum_get(bbone.irc_addr);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		spinlock_unlock(&irq->lock);
	} else {
		printf("Spurious interrupt\n");
	}

	omap_irc_irq_ack(bbone.irc_addr);
}

static void bbone_frame_init(void)
{
}

static void bbone_output_init(void)
{
#ifdef CONFIG_OMAP_UART
	const bool ok = omap_uart_init(&bbone.uart,
	    AM335x_UART0_IRQ, AM335x_UART0_BASE_ADDRESS,
	    AM335x_UART0_SIZE);

	if (ok)
		stdout_wire(&bbone.uart.outdev);
#endif
}

static void bbone_input_init(void)
{
#ifdef CONFIG_OMAP_UART
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);
		omap_uart_input_wire(&bbone.uart, srln);
		omap_irc_enable(bbone.irc_addr, AM335x_UART0_IRQ);
	}
#endif
}

size_t bbone_get_irq_count(void)
{
	return AM335x_IRC_IRQ_COUNT;
}

const char *bbone_get_platform_name(void)
{
	return "beaglebone";
}

/**
 * @}
 */

