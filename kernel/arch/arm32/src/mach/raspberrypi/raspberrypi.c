/*
 * Copyright (c) 2013 Beniamino Galvani
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

/** @addtogroup kernel_arm32_raspberrypi
 * @{
 */
/** @file
 *  @brief Raspberry PI platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/raspberrypi/raspberrypi.h>
#include <assert.h>
#include <genarch/drivers/pl011/pl011.h>
#include <genarch/drivers/bcm2835/irc.h>
#include <genarch/drivers/bcm2835/timer.h>
#include <genarch/drivers/bcm2835/mbox.h>
#include <arch/mm/page.h>
#include <mm/page.h>
#include <mm/km.h>
#include <genarch/fb/fb.h>
#include <abi/fb/visuals.h>
#include <genarch/srln/srln.h>
#include <stdbool.h>
#include <sysinfo/sysinfo.h>
#include <interrupt.h>
#include <ddi/ddi.h>

#define RPI_DEFAULT_MEMORY_START	0
#define RPI_DEFAULT_MEMORY_SIZE		0x08000000
#define RPI_MEMORY_SKIP			0x8000

static void raspberrypi_init(void);
static void raspberrypi_timer_irq_start(void);
static void raspberrypi_cpu_halt(void);
static void raspberrypi_get_memory_extents(uintptr_t *start, size_t *size);
static void raspberrypi_irq_exception(unsigned int exc_no, istate_t *istate);
static void raspberrypi_frame_init(void);
static void raspberrypi_output_init(void);
static void raspberrypi_input_init(void);
static size_t raspberrypi_get_irq_count(void);
static const char *raspberrypi_get_platform_name(void);

static struct {
	pl011_uart_t	uart;
	bcm2835_irc_t	*irc;
	bcm2835_timer_t	*timer;
} raspi;

struct arm_machine_ops raspberrypi_machine_ops = {
	raspberrypi_init,
	raspberrypi_timer_irq_start,
	raspberrypi_cpu_halt,
	raspberrypi_get_memory_extents,
	raspberrypi_irq_exception,
	raspberrypi_frame_init,
	raspberrypi_output_init,
	raspberrypi_input_init,
	raspberrypi_get_irq_count,
	raspberrypi_get_platform_name
};

static irq_ownership_t raspberrypi_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void raspberrypi_timer_irq_handler(irq_t *irq)
{
	bcm2835_timer_irq_ack(raspi.timer);
	irq_spinlock_unlock(&irq->lock, false);
	clock();
	irq_spinlock_lock(&irq->lock, false);
}

static void raspberrypi_init(void)
{
	/* Initialize interrupt controller */
	raspi.irc = (void *) km_map(BCM2835_IRC_ADDR, sizeof(bcm2835_irc_t),
	    KM_NATURAL_ALIGNMENT, PAGE_NOT_CACHEABLE);
	assert(raspi.irc);
	bcm2835_irc_init(raspi.irc);

	/* Initialize system timer */
	raspi.timer = (void *) km_map(BCM2835_TIMER_ADDR,
	    sizeof(bcm2835_timer_t), KM_NATURAL_ALIGNMENT, PAGE_NOT_CACHEABLE);
}

static void raspberrypi_timer_irq_start(void)
{
	/* Initialize timer IRQ */
	static irq_t timer_irq;
	irq_initialize(&timer_irq);
	timer_irq.inr = BCM2835_TIMER1_IRQ;
	timer_irq.claim = raspberrypi_timer_irq_claim;
	timer_irq.handler = raspberrypi_timer_irq_handler;
	irq_register(&timer_irq);

	bcm2835_irc_enable(raspi.irc, BCM2835_TIMER1_IRQ);
	bcm2835_timer_start(raspi.timer);
}

static void raspberrypi_cpu_halt(void)
{
	while (true)
		;
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void raspberrypi_get_memory_extents(uintptr_t *start, size_t *size)
{
	uint32_t mbase, msize;

	if (bcm2835_prop_get_memory(&mbase, &msize)) {
		*start = mbase + RPI_MEMORY_SKIP;
		*size  = msize - RPI_MEMORY_SKIP;
	} else {
		/* Stick to safe default values */
		*start = RPI_DEFAULT_MEMORY_START + RPI_MEMORY_SKIP;
		*size  = RPI_DEFAULT_MEMORY_SIZE - RPI_MEMORY_SKIP;
	}
}

static void raspberrypi_irq_exception(unsigned int exc_no, istate_t *istate)
{
	const unsigned inum = bcm2835_irc_inum_get(raspi.irc);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
		/* Spurious interrupt. */
		printf("cpu%d: spurious interrupt (inum=%d)\n", CPU->id, inum);
		bcm2835_irc_disable(raspi.irc, inum);
	}
}

static void raspberrypi_frame_init(void)
{
}

static void raspberrypi_output_init(void)
{
#ifdef CONFIG_FB
	uint32_t width, height;
	fb_properties_t prop;

	if (!bcm2835_mbox_get_fb_size(&width, &height)) {
		printf("mbox: could not get the framebuffer size\n");
		width = 640;
		height = 480;
	}
	if (bcm2835_fb_init(&prop, width, height)) {
		outdev_t *fb_dev = fb_init(&prop);
		if (fb_dev)
			stdout_wire(fb_dev);
	}
#endif

#ifdef CONFIG_PL011_UART
	if (pl011_uart_init(&raspi.uart, BCM2835_UART_IRQ,
	    BCM2835_UART0_BASE_ADDRESS))
		stdout_wire(&raspi.uart.outdev);
#endif
}

static void raspberrypi_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance) {
		indev_t *sink = stdin_wire();
		indev_t *srln = srln_wire(srln_instance, sink);

		pl011_uart_input_wire(&raspi.uart, srln);
		bcm2835_irc_enable(raspi.irc, BCM2835_UART_IRQ);
	}
}

size_t raspberrypi_get_irq_count(void)
{
	return BCM2835_IRQ_COUNT;
}

const char *raspberrypi_get_platform_name(void)
{
	return "raspberrypi";
}

/** @}
 */
