/*
 * Copyright (c) 2012 Matteo Facchinetti
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
#include <genarch/drivers/am335x/irc.h>
#include <genarch/drivers/am335x/uart.h>
#include <genarch/srln/srln.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <ddi/device.h>
#include <mm/km.h>

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
	am335x_irc_regs_t *irc_addr;
	am335x_uart_t uart;
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
	/* Initialize the interrupt controller */
	bbone.irc_addr = (void *) km_map(AM335x_IRC_BASE_ADDRESS,
	    AM335x_IRC_SIZE, PAGE_NOT_CACHEABLE);

	am335x_irc_init(bbone.irc_addr);
}

static void bbone_timer_irq_start(void)
{
}

static void bbone_cpu_halt(void)
{
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbone_get_memory_extents(uintptr_t *start, size_t *size)
{
}

static void bbone_irq_exception(unsigned int exc_no, istate_t *istate)
{
}

static void bbone_frame_init(void)
{
}

static void bbone_output_init(void)
{
	const bool ok = am335x_uart_init(&bbone.uart,
	    AM335x_UART0_IRQ, AM335x_UART0_BASE_ADDRESS,
	    AM335x_UART0_SIZE);

	if (ok)
		stdout_wire(&bbone.uart.outdev);
}

static void bbone_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);
		am335x_uart_input_wire(&bbone.uart, srln);
		am335x_irc_enable(bbone.irc_addr, AM335x_UART0_IRQ);
	}
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

