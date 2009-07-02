/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2009 Jiri Svoboda
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
/** @file
 */

#include <genarch/drivers/via-cuda/cuda.h>
#include <console/chardev.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <ddi/device.h>
#include <synch/spinlock.h>

static void cuda_packet_handle(cuda_instance_t *instance, size_t len);

/** B register fields */
enum {
	TREQ	= 0x08,
	TACK	= 0x10,
	TIP	= 0x20
};

/** IER register fields */
enum {
	IER_SET	= 0x80,
	SR_INT	= 0x04
};

static irq_ownership_t cuda_claim(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t ifr;

	ifr = pio_read_8(&dev->ifr);

	if ((ifr & SR_INT) != 0)
		return IRQ_ACCEPT;
	else
		return IRQ_DECLINE;
}

static void cuda_irq_handler(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t b, data;
	size_t pos;

	spinlock_lock(&instance->dev_lock);

	/* We have received one or more CUDA packets. Process them all. */
	while (true) {
		b = pio_read_8(&dev->b);

		if ((b & TREQ) != 0)
			break;	/* No data */

		pio_write_8(&dev->b, b & ~TIP);

		/* Read one packet. */

		pos = 0;
		do {
			data = pio_read_8(&dev->sr);
			b = pio_read_8(&dev->b);
			pio_write_8(&dev->b, b ^ TACK);

			if (pos < CUDA_RCV_BUF_SIZE)
				instance->rcv_buf[pos++] = data;
		} while ((b & TREQ) == 0);

		pio_write_8(&dev->b, b | TACK | TIP);

		cuda_packet_handle(instance, pos);
	}

	spinlock_unlock(&instance->dev_lock);
}

static void cuda_packet_handle(cuda_instance_t *instance, size_t len)
{
	uint8_t *data = instance->rcv_buf;

	if (data[0] != 0x00 || data[1] != 0x40 || data[2] != 0x2c)
		return;

	/* The packet contains one or two scancodes. */
	if (data[3] != 0xff)
		indev_push_character(instance->kbrdin, data[3]);		
	if (data[4] != 0xff)
		indev_push_character(instance->kbrdin, data[4]);
}

cuda_instance_t *cuda_init(cuda_t *dev, inr_t inr, cir_t cir, void *cir_arg)
{
	cuda_instance_t *instance
	    = malloc(sizeof(cuda_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->cuda = dev;
		instance->kbrdin = NULL;

		spinlock_initialize(&instance->dev_lock, "cuda_dev");

		irq_initialize(&instance->irq);
		instance->irq.devno = device_assign_devno();
		instance->irq.inr = inr;
		instance->irq.claim = cuda_claim;
		instance->irq.handler = cuda_irq_handler;
		instance->irq.instance = instance;
		instance->irq.cir = cir;
		instance->irq.cir_arg = cir_arg;
	}
	
	return instance;
}

void cuda_wire(cuda_instance_t *instance, indev_t *kbrdin)
{
	ASSERT(instance);
	ASSERT(kbrdin);

	instance->kbrdin = kbrdin;
	irq_register(&instance->irq);

	/* Enable SR interrupt. */
	pio_write_8(&instance->cuda->ier, IER_SET | SR_INT);
}

/** @}
 */
