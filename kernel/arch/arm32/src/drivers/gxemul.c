/*
 * Copyright (c) 2007 Michal Kebrt, Petr Stepan
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
 *  @brief GXemul drivers.
 */

#include <interrupt.h>
#include <ipc/irq.h>
#include <console/chardev.h>
#include <arch/drivers/gxemul.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <print.h>
#include <ddi/device.h>
#include <mm/page.h>
#include <arch/machine.h>
#include <arch/debug/print.h>

/* Addresses of devices. */
#define GXEMUL_VIDEORAM            0x10000000
#define GXEMUL_KBD                 0x10000000
#define GXEMUL_HALT_OFFSET         0x10
#define GXEMUL_RTC                 0x15000000
#define GXEMUL_RTC_FREQ_OFFSET     0x100
#define GXEMUL_RTC_ACK_OFFSET      0x110
#define GXEMUL_IRQC                0x16000000
#define GXEMUL_IRQC_MASK_OFFSET    0x4
#define GXEMUL_IRQC_UNMASK_OFFSET  0x8
#define GXEMUL_MP                  0x11000000
#define GXEMUL_MP_MEMSIZE_OFFSET   0x0090
#define GXEMUL_FB                  0x12000000

/* IRQs */
#define GXEMUL_KBD_IRQ		2
#define GXEMUL_TIMER_IRQ	4

static gxemul_hw_map_t gxemul_hw_map;
static chardev_t console;
static irq_t gxemul_console_irq;
static irq_t gxemul_timer_irq;

static bool hw_map_init_called = false;

static void gxemul_kbd_enable(chardev_t *dev);
static void gxemul_kbd_disable(chardev_t *dev);
static void gxemul_write(chardev_t *dev, const char ch, bool silent);
static char gxemul_do_read(chardev_t *dev);

static chardev_operations_t gxemul_ops = {
	.resume = gxemul_kbd_enable,
	.suspend = gxemul_kbd_disable,
	.write = gxemul_write,
	.read = gxemul_do_read,
};


/** Returns the mask of active interrupts. */
static inline uint32_t gxemul_irqc_get_sources(void)
{
	return *((uint32_t *) gxemul_hw_map.irqc);
}


/** Masks interrupt.
 * 
 * @param irq interrupt number
 */
static inline void gxemul_irqc_mask(uint32_t irq)
{
	*((uint32_t *) gxemul_hw_map.irqc_mask) = irq;
}


/** Unmasks interrupt.
 * 
 * @param irq interrupt number
 */
static inline void gxemul_irqc_unmask(uint32_t irq)
{
	*((uint32_t *) gxemul_hw_map.irqc_unmask) = irq;
}


/** Initializes #gxemul_hw_map. */
void gxemul_hw_map_init(void)
{
	gxemul_hw_map.videoram = hw_map(GXEMUL_VIDEORAM, PAGE_SIZE);
	gxemul_hw_map.kbd = hw_map(GXEMUL_KBD, PAGE_SIZE);
	gxemul_hw_map.rtc = hw_map(GXEMUL_RTC, PAGE_SIZE);
	gxemul_hw_map.irqc = hw_map(GXEMUL_IRQC, PAGE_SIZE);

	gxemul_hw_map.rtc_freq = gxemul_hw_map.rtc + GXEMUL_RTC_FREQ_OFFSET;
	gxemul_hw_map.rtc_ack = gxemul_hw_map.rtc + GXEMUL_RTC_ACK_OFFSET;
	gxemul_hw_map.irqc_mask = gxemul_hw_map.irqc + GXEMUL_IRQC_MASK_OFFSET;
	gxemul_hw_map.irqc_unmask = gxemul_hw_map.irqc +
	    GXEMUL_IRQC_UNMASK_OFFSET;

	hw_map_init_called = true;
}


/** Putchar that works with gxemul. 
 *
 * @param dev Not used.
 * @param ch Characted to be printed.
 */
static void gxemul_write(chardev_t *dev, const char ch, bool silent)
{
	if (!silent)
		*((char *) gxemul_hw_map.videoram) = ch;
}

/** Enables gxemul keyboard (interrupt unmasked).
 *
 * @param dev Not used.
 *
 * Called from getc(). 
 */
static void gxemul_kbd_enable(chardev_t *dev)
{
	gxemul_irqc_unmask(GXEMUL_KBD_IRQ);
}

/** Disables gxemul keyboard (interrupt masked).
 *
 * @param dev not used
 *
 * Called from getc().
 */
static void gxemul_kbd_disable(chardev_t *dev)
{
	gxemul_irqc_mask(GXEMUL_KBD_IRQ);
}

/** Read character using polling, assume interrupts disabled.
 *
 *  @param dev Not used.
 */
static char gxemul_do_read(chardev_t *dev)
{
	char ch;

	while (1) {
		ch = *((volatile char *) gxemul_hw_map.kbd);
		if (ch) {
			if (ch == '\r')
				return '\n';
			if (ch == 0x7f)
				return '\b';
			return ch;
		}
	}
}

/** Process keyboard interrupt. 
 *  
 *  @param irq IRQ information.
 *  @param arg Not used.
 */
static void gxemul_irq_handler(irq_t *irq, void *arg, ...)
{
	if ((irq->notif_cfg.notify) && (irq->notif_cfg.answerbox)) {
		ipc_irq_send_notif(irq);
	} else {
		char ch = 0;
		
		ch = *((char *) gxemul_hw_map.kbd);
		if (ch == '\r') {
			ch = '\n';
		}
		if (ch == 0x7f) {
			ch = '\b';
		}
		chardev_push_character(&console, ch);
	}
}

static irq_ownership_t gxemul_claim(void)
{
	return IRQ_ACCEPT;
}


/** Acquire console back for kernel. */
void gxemul_grab_console(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&gxemul_console_irq.lock);
	gxemul_console_irq.notif_cfg.notify = false;
	spinlock_unlock(&gxemul_console_irq.lock);
	interrupts_restore(ipl);
}

/** Return console to userspace. */
void gxemul_release_console(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&gxemul_console_irq.lock);
	if (gxemul_console_irq.notif_cfg.answerbox) {
		gxemul_console_irq.notif_cfg.notify = true;
	}
	spinlock_unlock(&gxemul_console_irq.lock);
	interrupts_restore(ipl);
}

/** Initializes console object representing gxemul console.
 *
 *  @param devno device number.
 */
void gxemul_console_init(devno_t devno)
{
	chardev_initialize("gxemul_console", &console, &gxemul_ops);
	stdin = &console;
	stdout = &console;
	
	irq_initialize(&gxemul_console_irq);
	gxemul_console_irq.devno = devno;
	gxemul_console_irq.inr = GXEMUL_KBD_IRQ;
	gxemul_console_irq.claim = gxemul_claim;
	gxemul_console_irq.handler = gxemul_irq_handler;
	irq_register(&gxemul_console_irq);
	
	gxemul_irqc_unmask(GXEMUL_KBD_IRQ);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, GXEMUL_KBD_IRQ);
	sysinfo_set_item_val("kbd.address.virtual", NULL, gxemul_hw_map.kbd);
}

/** Starts gxemul Real Time Clock device, which asserts regular interrupts.
 * 
 * @param frequency Interrupts frequency (0 disables RTC).
 */
static void gxemul_timer_start(uint32_t frequency)
{
	*((uint32_t*) gxemul_hw_map.rtc_freq) = frequency;
}

static irq_ownership_t gxemul_timer_claim(void)
{
	return IRQ_ACCEPT;
}

/** Timer interrupt handler.
 *
 * @param irq Interrupt information.
 * @param arg Not used.
 */
static void gxemul_timer_irq_handler(irq_t *irq, void *arg, ...)
{
	/*
	* We are holding a lock which prevents preemption.
	* Release the lock, call clock() and reacquire the lock again.
	*/
	spinlock_unlock(&irq->lock);
	clock();
	spinlock_lock(&irq->lock);

	/* acknowledge tick */
	*((uint32_t*) gxemul_hw_map.rtc_ack) = 0;
}

/** Initializes and registers timer interrupt handler. */
static void gxemul_timer_irq_init(void)
{
	irq_initialize(&gxemul_timer_irq);
	gxemul_timer_irq.devno = device_assign_devno();
	gxemul_timer_irq.inr = GXEMUL_TIMER_IRQ;
	gxemul_timer_irq.claim = gxemul_timer_claim;
	gxemul_timer_irq.handler = gxemul_timer_irq_handler;

	irq_register(&gxemul_timer_irq);
}


/** Starts timer.
 *
 * Initiates regular timer interrupts after initializing
 * corresponding interrupt handler.
 */
void gxemul_timer_irq_start(void)
{
	gxemul_timer_irq_init();
	gxemul_timer_start(GXEMUL_TIMER_FREQ);
}

/** Returns the size of emulated memory.
 *
 * @return Size in bytes.
 */
size_t gxemul_get_memory_size(void) 
{
	return  *((int *) (GXEMUL_MP + GXEMUL_MP_MEMSIZE_OFFSET));
}

/** Prints a character. 
 *
 *  @param ch Character to be printed.
 */
void gxemul_debug_putc(char ch)
{
	char *addr = 0;
	if (!hw_map_init_called) {
	 	addr = (char *) GXEMUL_KBD;
	} else {
		addr = (char *) gxemul_hw_map.videoram;
	}

	*(addr) = ch;
}

/** Stops gxemul. */
void gxemul_cpu_halt(void)
{
	char * addr = 0;
	if (!hw_map_init_called) {
	 	addr = (char *) GXEMUL_KBD;
	} else {
		addr = (char *) gxemul_hw_map.videoram;
	}
	
	*(addr + GXEMUL_HALT_OFFSET) = '\0';
}

/** Gxemul specific interrupt exception handler.
 *
 * Determines sources of the interrupt from interrupt controller and
 * calls high-level handlers for them.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
void gxemul_irq_exception(int exc_no, istate_t *istate)
{
	uint32_t sources = gxemul_irqc_get_sources();
	int i;
	
	for (i = 0; i < GXEMUL_IRQC_MAX_IRQ; i++) {
		if (sources & (1 << i)) {
			irq_t *irq = irq_dispatch_and_lock(i);
			if (irq) {
				/* The IRQ handler was found. */
				irq->handler(irq, irq->arg);
				spinlock_unlock(&irq->lock);
			} else {
				/* Spurious interrupt.*/
				dprintf("cpu%d: spurious interrupt (inum=%d)\n",
				    CPU->id, i);
			}
		}
	}
}

/** Returns address of framebuffer device.
 *
 *  @return Address of framebuffer device.
 */
uintptr_t gxemul_get_fb_address(void)
{
	return (uintptr_t) GXEMUL_FB;
}

/** @}
 */
