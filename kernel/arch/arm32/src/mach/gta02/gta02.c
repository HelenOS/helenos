/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup arm32gxemul
 * @{
 */
/** @file
 *  @brief Openmoko GTA02 (Neo FreeRunner) platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/gta02/gta02.h>
#include <arch/mm/page.h>
#include <mm/page.h>
#include <genarch/fb/fb.h>
#include <genarch/fb/visuals.h>
#include <genarch/drivers/s3c24xx_uart/s3c24xx_uart.h>
#include <genarch/drivers/s3c24xx_irqc/s3c24xx_irqc.h>
#include <genarch/drivers/s3c24xx_timer/s3c24xx_timer.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <ddi/device.h>

#define GTA02_MEMORY_START	0x30000000	/* physical */
#define GTA02_MEMORY_SIZE	0x08000000	/* 128 MB */
#define GTA02_MEMORY_SKIP	0x8000

/** GTA02 serial console UART address (UART S3C24XX CPU UART channel 2). */
#define GTA02_SCONS_BASE	0x50008000

/** GTA02 framebuffer base address */
#define GTA02_FB_BASE		0x08800000

/** IRQ number used for clock */
#define GTA02_TIMER_IRQ		S3C24XX_INT_TIMER0

static void gta02_init(void);
static void gta02_timer_irq_start(void);
static void gta02_cpu_halt(void);
static void gta02_get_memory_extents(uintptr_t *start, uintptr_t *size);
static void gta02_irq_exception(unsigned int exc_no, istate_t *istate);
static void gta02_frame_init(void);
static void gta02_output_init(void);
static void gta02_input_init(void);

static void gta02_timer_irq_init(void);
static void gta02_timer_start(void);
static irq_ownership_t gta02_timer_irq_claim(irq_t *irq);
static void gta02_timer_irq_handler(irq_t *irq);

static void *gta02_scons_out;
static s3c24xx_irqc_t *gta02_irqc;
static s3c24xx_timer_t *gta02_timer;

static irq_t gta02_timer_irq;

struct arm_machine_ops gta02_machine_ops = {
	gta02_init,
	gta02_timer_irq_start,
	gta02_cpu_halt,
	gta02_get_memory_extents,
	gta02_irq_exception,
	gta02_frame_init,
	gta02_output_init,
	gta02_input_init
};

static void gta02_init(void)
{
	gta02_scons_out = (void *) hw_map(GTA02_SCONS_BASE, PAGE_SIZE);
	gta02_irqc = (void *) hw_map(S3C24XX_IRQC_ADDRESS, PAGE_SIZE);
	gta02_timer = (void *) hw_map(S3C24XX_TIMER_ADDRESS, PAGE_SIZE);

	/* Make all interrupt sources use IRQ mode (not FIQ). */
	pio_write_32(&gta02_irqc->intmod, 0x00000000);

	/* Disable all interrupt sources. */
	pio_write_32(&gta02_irqc->intmsk, 0xffffffff);

	/* Disable interrupts from all sub-sources. */
	pio_write_32(&gta02_irqc->intsubmsk, 0xffffffff);
}

static void gta02_timer_irq_start(void)
{
	gta02_timer_irq_init();
	gta02_timer_start();
}

static void gta02_cpu_halt(void)
{
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void gta02_get_memory_extents(uintptr_t *start, uintptr_t *size)
{
	*start = GTA02_MEMORY_START + GTA02_MEMORY_SKIP;
	*size  = GTA02_MEMORY_SIZE - GTA02_MEMORY_SKIP;
}

static void gta02_irq_exception(unsigned int exc_no, istate_t *istate)
{
	uint32_t inum;

	inum = pio_read_32(&gta02_irqc->intoffset);

	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		spinlock_unlock(&irq->lock);
	} else {
		/* Spurious interrupt.*/
		printf("cpu%d: spurious interrupt (inum=%d)\n",
		    CPU->id, inum);
	}

	/* Clear interrupt condition in the interrupt controller. */
	pio_write_32(&gta02_irqc->srcpnd, S3C24XX_INT_BIT(inum));
	pio_write_32(&gta02_irqc->intpnd, S3C24XX_INT_BIT(inum));
}

static void gta02_frame_init(void)
{
}

static void gta02_output_init(void)
{
#ifdef CONFIG_FB
	parea_t fb_parea;

	fb_properties_t prop = {
		.addr = GTA02_FB_BASE,
		.offset = 0,
		.x = 480,
		.y = 640,
		.scan = 960,
		.visual = VISUAL_RGB_5_6_5_LE
	};

	outdev_t *fb_dev = fb_init(&prop);
	if (fb_dev) {
		stdout_wire(fb_dev);
		fb_parea.pbase = GTA02_FB_BASE;
		fb_parea.frames = 150;
		ddi_parea_register(&fb_parea);
	}
#endif
	outdev_t *scons_dev;

	scons_dev = s3c24xx_uart_init((ioport8_t *) gta02_scons_out);
	if (scons_dev)
		stdout_wire(scons_dev);
}

static void gta02_input_init(void)
{
}

static void gta02_timer_irq_init(void)
{
	irq_initialize(&gta02_timer_irq);
	gta02_timer_irq.devno = device_assign_devno();
	gta02_timer_irq.inr = GTA02_TIMER_IRQ;
	gta02_timer_irq.claim = gta02_timer_irq_claim;
	gta02_timer_irq.handler = gta02_timer_irq_handler;

	irq_register(&gta02_timer_irq);
}

static irq_ownership_t gta02_timer_irq_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void gta02_timer_irq_handler(irq_t *irq)
{
	/*
	 * We are holding a lock which prevents preemption.
	 * Release the lock, call clock() and reacquire the lock again.
	 */
	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);
}

static void gta02_timer_start(void)
{
	s3c24xx_timer_t *timer = gta02_timer;

	/*
	 * See S3C2442B user manual chapter 10 (PWM Timer) for description
	 * of timer operation. Starting a timer is described in the
	 * section 'Timer initialization using manual update bit and
	 * inverter bit'.
	 */

	/*
	 * GTA02 PCLK should be 100 MHz.
	 * Timer input freq. = PCLK / divider / (1+prescaler)
	 * 100 MHz / 2 / (1+7) / 62500 ~= 100 Hz
	 */
#if HZ != 100
#warning Other HZ than 100 not suppored.
#endif

	/* Set prescaler values. No pre-divison, no dead zone. */
	pio_write_32(&timer->tcfg0, 7); /* prescale 1/8 */

	/* No DMA request, divider value = 2 for all timers. */
	pio_write_32(&timer->tcfg1, 0);

	/* Stop all timers. */
	pio_write_32(&timer->tcon, 0);

	/* Start counting from 64k-1. Compare value is irrelevant. */
	pio_write_32(&timer->timer[0].cntb, 62500);
	pio_write_32(&timer->timer[0].cmpb, 0);

	/* Enable interrupts from timer0 */
	pio_write_32(&gta02_irqc->intmsk, pio_read_32(&gta02_irqc->intmsk) &
	    ~S3C24XX_INT_BIT(S3C24XX_INT_TIMER0));

	/* Load data from tcntb0/tcmpb0 into tcnt0/tcmp0. */
	pio_write_32(&timer->tcon, TCON_T0_AUTO_RLD | TCON_T0_MUPDATE);

	/* Start timer 0. Inverter is off. */
	pio_write_32(&timer->tcon, TCON_T0_AUTO_RLD | TCON_T0_START);
}

/** @}
 */
