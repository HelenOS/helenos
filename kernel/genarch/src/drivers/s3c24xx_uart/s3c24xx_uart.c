/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 *
 * This UART is present on the Samsung S3C24xx CPU (on the gta02 platform).
 */

#include <genarch/drivers/s3c24xx_uart/s3c24xx_uart.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/device.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

/** S3C24xx UART register offsets */
#define S3C24XX_ULCON			0x00
#define S3C24XX_UCON			0x04
#define S3C24XX_UFCON			0x08
#define S3C24XX_UMCON			0x0c
#define S3C24XX_UTRSTAT			0x10
#define S3C24XX_UERSTAT			0x14
#define S3C24XX_UFSTAT			0x18
#define S3C24XX_UMSTAT			0x1c
#define S3C24XX_UTXH			0x20
#define S3C24XX_URXH			0x24
#define S3C24XX_UBRDIV			0x28

/* Bits in UTRSTAT register */
#define S3C24XX_UTRSTAT_TX_EMPTY	0x4
#define S3C24XX_UTRSTAT_RDATA		0x1

static void s3c24xx_uart_sendb(outdev_t *dev, uint8_t byte)
{
	s3c24xx_uart_instance_t *instance =
	    (s3c24xx_uart_instance_t *) dev->data;
	ioport32_t *utrstat, *utxh;

	utrstat = (ioport32_t *) (instance->base + S3C24XX_UTRSTAT);
	utxh = (ioport32_t *) (instance->base + S3C24XX_UTXH);

	/* Wait for transmitter to be empty. */
	while ((pio_read_32(utrstat) & S3C24XX_UTRSTAT_TX_EMPTY) == 0)
		;

	pio_write_32(utxh, byte);
}

static void s3c24xx_uart_putchar(outdev_t *dev, wchar_t ch, bool silent)
{
	if (!silent) {
		if (!ascii_check(ch)) {
			s3c24xx_uart_sendb(dev, U_SPECIAL);
		} else {
    			if (ch == '\n')
				s3c24xx_uart_sendb(dev, (uint8_t) '\r');
			s3c24xx_uart_sendb(dev, (uint8_t) ch);
		}
	}
}

static irq_ownership_t s3c24xx_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void s3c24xx_uart_irq_handler(irq_t *irq)
{
	s3c24xx_uart_instance_t *instance = irq->instance;
	ioport32_t *utrstat, *urxh;

	utrstat = (ioport32_t *) (instance->base + S3C24XX_UTRSTAT);
	urxh = (ioport32_t *) (instance->base + S3C24XX_URXH);

	if ((pio_read_32(utrstat) & S3C24XX_UTRSTAT_RDATA) != 0) {
		uint32_t data = pio_read_32(urxh);
		indev_push_character(instance->indev, data & 0xff);
	}
}

static outdev_operations_t s3c24xx_uart_ops = {
	.write = s3c24xx_uart_putchar,
	.redraw = NULL
};

outdev_t *s3c24xx_uart_init(ioport8_t *base, inr_t inr)
{
	outdev_t *uart_dev = malloc(sizeof(outdev_t), FRAME_ATOMIC);
	if (!uart_dev)
		return NULL;

	s3c24xx_uart_instance_t *instance =
	    malloc(sizeof(s3c24xx_uart_instance_t), FRAME_ATOMIC);
	if (!instance) {
		free(uart_dev);
		return NULL;
	}

	outdev_initialize("s3c24xx_uart_dev", uart_dev, &s3c24xx_uart_ops);
	uart_dev->data = instance;

	instance->base = base;
	instance->indev = NULL;

	/* Initialize IRQ structure. */
	irq_initialize(&instance->irq);
	instance->irq.devno = device_assign_devno();
	instance->irq.inr = inr;
	instance->irq.claim = s3c24xx_uart_claim;
	instance->irq.handler = s3c24xx_uart_irq_handler;
	instance->irq.instance = instance;

	/* Disable FIFO */
	ioport32_t *ufcon;
	ufcon = (ioport32_t *) (instance->base + S3C24XX_UFCON);
	pio_write_32(ufcon, pio_read_32(ufcon) & ~0x01);

	/* Set RX interrupt to pulse mode */
	ioport32_t *ucon;
	ucon = (ioport32_t *) (instance->base + S3C24XX_UCON);
	pio_write_32(ucon, pio_read_32(ucon) & ~(1 << 8));

	if (!fb_exported) {
		/*
		 * This is the necessary evil until the userspace driver is entirely
		 * self-sufficient.
		 */
		sysinfo_set_item_val("fb", NULL, true);
		sysinfo_set_item_val("fb.kind", NULL, 3);
		sysinfo_set_item_val("fb.address.physical", NULL, KA2PA(base));

		fb_exported = true;
	}

	return uart_dev;
}

void s3c24xx_uart_input_wire(s3c24xx_uart_instance_t *instance, indev_t *indev)
{
	ASSERT(instance);
	ASSERT(indev);

	instance->indev = indev;
	irq_register(&instance->irq);
}

/** @}
 */
