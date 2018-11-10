/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief	pl050 keyboard/mouse driver.
 *
 * It takes care of low-level keyboard functions.
 */

#include <genarch/drivers/pl050/pl050.h>
#include <arch/asm.h>
#include <console/chardev.h>
#include <stdlib.h>

#define PL050_KEY_RELEASE 0xF0
#define PL050_ESC_KEY	0xE0
#define PL050_CAPS_SCAN_CODE   0x58

/** Structure for pl050's IRQ. */
static pl050_t *pl050;

static irq_ownership_t pl050_claim(irq_t *irq)
{
	uint8_t status;
	if ((status = pio_read_8(pl050->status)) & PL050_STAT_RXFULL)
		return IRQ_ACCEPT;
	else {
		return IRQ_DECLINE;
	}
}

static void pl050_irq_handler(irq_t *irq)
{
	uint8_t data;
	uint8_t status;
	pl050_instance_t *instance = irq->instance;

	while ((status = pio_read_8(pl050->status)) & PL050_STAT_RXFULL) {
		data = pio_read_8(pl050->data);
		indev_push_character(instance->kbrdin, data);

	}
}

/** Initialize pl050. */
pl050_instance_t *pl050_init(pl050_t *dev, inr_t inr)
{

	pl050_instance_t *instance =
	    malloc(sizeof(pl050_instance_t));

	pl050 = dev;

	if (instance) {
		instance->pl050 = dev;
		instance->kbrdin = NULL;

		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = pl050_claim;
		instance->irq.handler = pl050_irq_handler;
		instance->irq.instance = instance;
	}

	return instance;
}

void pl050_wire(pl050_instance_t *instance, indev_t *kbrdin)
{
	uint8_t val;

	instance->kbrdin = kbrdin;
	irq_register(&instance->irq);

	val = PL050_CR_RXINTR | PL050_CR_INTR;

	pio_write_8(pl050->ctrl, val);

	/* reset the data buffer */
	pio_read_8(pl050->data);

}

/** @}
 */
