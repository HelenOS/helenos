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

#include <assert.h>
#include <genarch/drivers/via-cuda/cuda.h>
#include <console/chardev.h>
#include <ddi/irq.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <synch/spinlock.h>
#include <mem.h>

static irq_ownership_t cuda_claim(irq_t *irq);
static void cuda_irq_handler(irq_t *irq);

static void cuda_irq_listen(irq_t *irq);
static void cuda_irq_receive(irq_t *irq);
static void cuda_irq_rcv_end(irq_t *irq, void *buf, size_t *len);
static void cuda_irq_send_start(irq_t *irq);
static void cuda_irq_send(irq_t *irq);

static void cuda_packet_handle(cuda_instance_t *instance, uint8_t *buf, size_t len);
static void cuda_send_start(cuda_instance_t *instance);
static void cuda_autopoll_set(cuda_instance_t *instance, bool enable);

/** B register fields */
enum {
	TREQ	= 0x08,
	TACK	= 0x10,
	TIP	= 0x20
};

/** IER register fields */
enum {
	IER_CLR	= 0x00,
	IER_SET	= 0x80,

	SR_INT	= 0x04,
	ALL_INT	= 0x7f
};

/** ACR register fields */
enum {
	SR_OUT	= 0x10
};

/** Packet types */
enum {
	PT_ADB	= 0x00,
	PT_CUDA	= 0x01
};

/** CUDA packet types */
enum {
	CPT_AUTOPOLL	= 0x01
};

cuda_instance_t *cuda_init(cuda_t *dev, inr_t inr, cir_t cir, void *cir_arg)
{
	cuda_instance_t *instance =
	    malloc(sizeof(cuda_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->cuda = dev;
		instance->kbrdin = NULL;
		instance->xstate = cx_listen;
		instance->bidx = 0;
		instance->snd_bytes = 0;

		spinlock_initialize(&instance->dev_lock, "cuda.instance.dev_lock");

		/* Disable all interrupts from CUDA. */
		pio_write_8(&dev->ier, IER_CLR | ALL_INT);

		irq_initialize(&instance->irq);
		instance->irq.inr = inr;
		instance->irq.claim = cuda_claim;
		instance->irq.handler = cuda_irq_handler;
		instance->irq.instance = instance;
		instance->irq.cir = cir;
		instance->irq.cir_arg = cir_arg;
		instance->irq.preack = true;
	}

	return instance;
}

#include <log.h>
void cuda_wire(cuda_instance_t *instance, indev_t *kbrdin)
{
	cuda_t *dev = instance->cuda;

	assert(instance);
	assert(kbrdin);

	instance->kbrdin = kbrdin;
	irq_register(&instance->irq);

	/* Enable SR interrupt. */
	pio_write_8(&dev->ier, TIP | TREQ);
	pio_write_8(&dev->ier, IER_SET | SR_INT);

	/* Enable ADB autopolling. */
	cuda_autopoll_set(instance, true);
}

static irq_ownership_t cuda_claim(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t ifr;

	spinlock_lock(&instance->dev_lock);
	ifr = pio_read_8(&dev->ifr);
	spinlock_unlock(&instance->dev_lock);

	if ((ifr & SR_INT) == 0)
		return IRQ_DECLINE;

	return IRQ_ACCEPT;
}

static void cuda_irq_handler(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	uint8_t rbuf[CUDA_RCV_BUF_SIZE];
	size_t len;
	bool handle;

	handle = false;
	len = 0;

	spinlock_lock(&instance->dev_lock);

	/* Lower IFR.SR_INT so that CUDA can generate next int by raising it. */
	pio_write_8(&instance->cuda->ifr, SR_INT);

	switch (instance->xstate) {
	case cx_listen:
		cuda_irq_listen(irq);
		break;
	case cx_receive:
		cuda_irq_receive(irq);
		break;
	case cx_rcv_end:
		cuda_irq_rcv_end(irq, rbuf, &len);
		handle = true;
		break;
	case cx_send_start:
		cuda_irq_send_start(irq);
		break;
	case cx_send:
		cuda_irq_send(irq);
		break;
	}

	spinlock_unlock(&instance->dev_lock);

	/* Handle an incoming packet. */
	if (handle)
		cuda_packet_handle(instance, rbuf, len);
}

/** Interrupt in listen state.
 *
 * Start packet reception.
 */
static void cuda_irq_listen(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t b;

	b = pio_read_8(&dev->b);

	if ((b & TREQ) != 0) {
		log(LF_OTHER, LVL_ERROR, "cuda_irq_listen: no TREQ?!");
		return;
	}

	pio_read_8(&dev->sr);
	pio_write_8(&dev->b, pio_read_8(&dev->b) & ~TIP);
	instance->xstate = cx_receive;
}

/** Interrupt in receive state.
 *
 * Receive next byte of packet.
 */
static void cuda_irq_receive(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t b, data;

	data = pio_read_8(&dev->sr);
	if (instance->bidx < CUDA_RCV_BUF_SIZE)
		instance->rcv_buf[instance->bidx++] = data;

	b = pio_read_8(&dev->b);

	if ((b & TREQ) == 0) {
		pio_write_8(&dev->b, b ^ TACK);
	} else {
		pio_write_8(&dev->b, b | TACK | TIP);
		instance->xstate = cx_rcv_end;
	}
}

/** Interrupt in rcv_end state.
 *
 * Terminate packet reception. Either go back to listen state or start
 * receiving another packet if CUDA has one for us.
 */
static void cuda_irq_rcv_end(irq_t *irq, void *buf, size_t *len)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t b;

	b = pio_read_8(&dev->b);
	pio_read_8(&dev->sr);

	if ((b & TREQ) == 0) {
		instance->xstate = cx_receive;
		pio_write_8(&dev->b, b & ~TIP);
	} else {
		instance->xstate = cx_listen;
		cuda_send_start(instance);
	}

	memcpy(buf, instance->rcv_buf, instance->bidx);
	*len = instance->bidx;
	instance->bidx = 0;
}

/** Interrupt in send_start state.
 *
 * Process result of sending first byte (and send second on success).
 */
static void cuda_irq_send_start(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;
	uint8_t b;

	b = pio_read_8(&dev->b);

	if ((b & TREQ) == 0) {
		/* Collision */
		pio_write_8(&dev->acr, pio_read_8(&dev->acr) & ~SR_OUT);
		pio_read_8(&dev->sr);
		pio_write_8(&dev->b, pio_read_8(&dev->b) | TIP | TACK);
		instance->xstate = cx_listen;
		return;
	}

	pio_write_8(&dev->sr, instance->snd_buf[1]);
	pio_write_8(&dev->b, pio_read_8(&dev->b) ^ TACK);
	instance->bidx = 2;

	instance->xstate = cx_send;
}

/** Interrupt in send state.
 *
 * Send next byte or terminate transmission.
 */
static void cuda_irq_send(irq_t *irq)
{
	cuda_instance_t *instance = irq->instance;
	cuda_t *dev = instance->cuda;

	if (instance->bidx < instance->snd_bytes) {
		/* Send next byte. */
		pio_write_8(&dev->sr, instance->snd_buf[instance->bidx++]);
		pio_write_8(&dev->b, pio_read_8(&dev->b) ^ TACK);
		return;
	}

	/* End transfer. */
	instance->snd_bytes = 0;
	instance->bidx = 0;

	pio_write_8(&dev->acr, pio_read_8(&dev->acr) & ~SR_OUT);
	pio_read_8(&dev->sr);
	pio_write_8(&dev->b, pio_read_8(&dev->b) | TACK | TIP);

	instance->xstate = cx_listen;
	/* TODO: Match reply with request. */
}

static void cuda_packet_handle(cuda_instance_t *instance, uint8_t *data, size_t len)
{
	if (data[0] != 0x00 || data[1] != 0x40 || (data[2] != 0x2c &&
	    data[2] != 0x8c))
		return;

	/* The packet contains one or two scancodes. */
	if (data[3] != 0xff)
		indev_push_character(instance->kbrdin, data[3]);
	if (data[4] != 0xff)
		indev_push_character(instance->kbrdin, data[4]);
}

static void cuda_autopoll_set(cuda_instance_t *instance, bool enable)
{
	instance->snd_buf[0] = PT_CUDA;
	instance->snd_buf[1] = CPT_AUTOPOLL;
	instance->snd_buf[2] = enable ? 0x01 : 0x00;
	instance->snd_bytes = 3;
	instance->bidx = 0;

	cuda_send_start(instance);
}

static void cuda_send_start(cuda_instance_t *instance)
{
	cuda_t *dev = instance->cuda;

	assert(instance->xstate == cx_listen);

	if (instance->snd_bytes == 0)
		return;

	/* Check for incoming data. */
	if ((pio_read_8(&dev->b) & TREQ) == 0)
		return;

	pio_write_8(&dev->acr, pio_read_8(&dev->acr) | SR_OUT);
	pio_write_8(&dev->sr, instance->snd_buf[0]);
	pio_write_8(&dev->b, pio_read_8(&dev->b) & ~TIP);

	instance->xstate = cx_send_start;
}


/** @}
 */
