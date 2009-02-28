/*
 * Copyright (c) 2009 Jakub Jermar
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
 * @brief	Zilog 8530 serial port driver.
 */

#include <genarch/kbd/z8530.h>
#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#include <genarch/kbd/scanc_sun.h>
#include <arch/drivers/kbd.h>
#include <console/console.h>
#include <console/chardev.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <mm/slab.h>

static inline void z8530_write(ioport8_t *ctl, uint8_t reg, uint8_t val)
{
	/*
	 * Registers 8-15 will automatically issue the Point High
	 * command as their bit 3 is 1.
	 */
	pio_write_8(ctl, reg);	/* select register */
	pio_write_8(ctl, val);	/* write value */
}

static inline uint8_t z8530_read(ioport8_t *ctl, uint8_t reg) 
{
	/*
	 * Registers 8-15 will automatically issue the Point High
	 * command as their bit 3 is 1.
	 */
	pio_write_8(ctl, reg);	/* select register */
	return pio_read_8(ctl);
}

/*
 * These codes read from z8530 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f		/* all keys up */

static void z8530_suspend(chardev_t *);
static void z8530_resume(chardev_t *);

static chardev_operations_t ops = {
	.suspend = z8530_suspend,
	.resume = z8530_resume,
};

/** Initialize z8530. */
bool
z8530_init(z8530_t *dev, devno_t devno, inr_t inr, cir_t cir, void *cir_arg)
{
	z8530_instance_t *instance;

	chardev_initialize("z8530_kbd", &kbrd, &ops);
	stdin = &kbrd;

	instance = malloc(sizeof(z8530_instance_t), FRAME_ATOMIC);
	if (!instance)
		return false;

	instance->devno = devno;
	instance->z8530 = dev;

	irq_initialize(&instance->irq);
	instance->irq.devno = devno;
	instance->irq.inr = inr;
	instance->irq.claim = z8530_claim;
	instance->irq.handler = z8530_irq_handler;
	instance->irq.instance = instance;
	instance->irq.cir = cir;
	instance->irq.cir_arg = cir_arg;
	irq_register(&instance->irq);

	(void) z8530_read(&dev->ctl_a, RR8);

	/*
	 * Clear any pending TX interrupts or we never manage
	 * to set FHC UART interrupt state to idle.
	 */
	z8530_write(&dev->ctl_a, WR0, WR0_TX_IP_RST);

	/* interrupt on all characters */
	z8530_write(&dev->ctl_a, WR1, WR1_IARCSC);

	/* 8 bits per character and enable receiver */
	z8530_write(&dev->ctl_a, WR3, WR3_RX8BITSCH | WR3_RX_ENABLE);
	
	/* Master Interrupt Enable. */
	z8530_write(&dev->ctl_a, WR9, WR9_MIE);

	return true;
}

/* Called from getc(). */
void z8530_resume(chardev_t *d)
{
}

/* Called from getc(). */
void z8530_suspend(chardev_t *d)
{
}

irq_ownership_t z8530_claim(irq_t *irq)
{
	z8530_instance_t *instance = irq->instance;
	z8530_t *dev = instance->z8530;

	return (z8530_read(&dev->ctl_a, RR0) & RR0_RCA);
}

void z8530_irq_handler(irq_t *irq)
{
	z8530_instance_t *instance = irq->instance;
	z8530_t *dev = instance->z8530;
	uint8_t x;

	if (z8530_read(&dev->ctl_a, RR0) & RR0_RCA) {
		x = z8530_read(&dev->ctl_a, RR8);
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
