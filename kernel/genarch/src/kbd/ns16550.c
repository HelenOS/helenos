/*
 * Copyright (c) 2001-2006 Jakub Jermar
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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	NS 16550 serial port / keyboard driver.
 */

#include <genarch/kbd/ns16550.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/kbd.h>
#include <ddi/irq.h>
#include <ipc/irq.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <console/chardev.h>
#include <console/console.h>
#include <interrupt.h>
#include <arch/interrupt.h>
#include <sysinfo/sysinfo.h>
#include <synch/spinlock.h>
#include <mm/slab.h>

#define LSR_DATA_READY	0x01

static irq_t *ns16550_irq;

/*
 * These codes read from ns16550 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

static void ns16550_suspend(chardev_t *);
static void ns16550_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = ns16550_suspend,
	.resume = ns16550_resume,
};

/** Initialize keyboard and service interrupts using kernel routine */
void ns16550_grab(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&ns16550_irq->lock);
	ns16550_irq->notif_cfg.notify = false;
	spinlock_unlock(&ns16550_irq->lock);
	interrupts_restore(ipl);
}

/** Resume the former interrupt vector */
void ns16550_release(void)
{
	ipl_t ipl = interrupts_disable();
	spinlock_lock(&ns16550_irq->lock);
	if (ns16550_irq->notif_cfg.answerbox)
		ns16550_irq->notif_cfg.notify = true;
	spinlock_unlock(&ns16550_irq->lock);
	interrupts_restore(ipl);
}

/** Initialize ns16550.
 *
 * @param dev		Addrress of the beginning of the device in I/O space.
 * @param devno		Device number.
 * @param inr		Interrupt number.
 * @param cir		Clear interrupt function.
 * @param cir_arg	First argument to cir.
 *
 * @return		True on success, false on failure.
 */
bool
ns16550_init(ns16550_t *dev, devno_t devno, inr_t inr, cir_t cir, void *cir_arg)
{
	ns16550_instance_t *instance;

	chardev_initialize("ns16550_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	instance = malloc(sizeof(ns16550_instance_t), FRAME_ATOMIC);
	if (!instance)
		return false;

	instance->devno = devno;
	instance->ns16550 = dev;
	
	irq_initialize(&instance->irq);
	instance->irq.devno = devno;
	instance->irq.inr = inr;
	instance->irq.claim = ns16550_claim;
	instance->irq.handler = ns16550_irq_handler;
	instance->irq.instance = instance;
	instance->irq.cir = cir;
	instance->irq.cir_arg = cir_arg;
	irq_register(&instance->irq);

	ns16550_irq = &instance->irq;	/* TODO: remove me soon */
	
	while ((pio_read_8(&dev->lsr) & LSR_DATA_READY))
		(void) pio_read_8(&dev->rbr);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.type", NULL, KBD_NS16550);
	sysinfo_set_item_val("kbd.devno", NULL, devno);
	sysinfo_set_item_val("kbd.inr", NULL, inr);
	sysinfo_set_item_val("kbd.address.virtual", NULL, (uintptr_t) dev);
	sysinfo_set_item_val("kbd.port", NULL, (uintptr_t) dev);
	
	/* Enable interrupts */
	pio_write_8(&dev->ier, IER_ERBFI);
	pio_write_8(&dev->mcr, MCR_OUT2);
	
	ns16550_grab();
	
	return true;
}

/* Called from getc(). */
void ns16550_resume(chardev_t *d)
{
}

/* Called from getc(). */
void ns16550_suspend(chardev_t *d)
{
}

irq_ownership_t ns16550_claim(irq_t *irq)
{
	ns16550_instance_t *ns16550_instance = irq->instance;
	ns16550_t *dev = ns16550_instance->ns16550;

	if (pio_read_8(&dev->lsr) & LSR_DATA_READY)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

void ns16550_irq_handler(irq_t *irq)
{
	if (irq->notif_cfg.notify && irq->notif_cfg.answerbox) {
		/*
		 * This will hopefully go to the IRQ dispatch code soon.
		 */
		ipc_irq_send_notif(irq);
		return;
	}

	ns16550_instance_t *ns16550_instance = irq->instance;
	ns16550_t *dev = ns16550_instance->ns16550;

	if (pio_read_8(&dev->lsr) & LSR_DATA_READY) {
		uint8_t x;
		
		x = pio_read_8(&dev->rbr);
		
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}

}

/** @}
 */
