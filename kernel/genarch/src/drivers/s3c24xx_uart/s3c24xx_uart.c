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
#include <arch/asm.h>
#include <mm/slab.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

/** S3C24xx UART register offsets */
#define S3C24XX_UTRSTAT		0x10
#define S3C24XX_UTXH		0x20

/* Bits in UTXH register */
#define S3C24XX_UTXH_TX_EMPTY	0x4

typedef struct {
	ioport8_t *base;
} s3c24xx_uart_instance_t;

static void s3c24xx_uart_sendb(outdev_t *dev, uint8_t byte)
{
	s3c24xx_uart_instance_t *instance =
	    (s3c24xx_uart_instance_t *) dev->data;
	ioport32_t *utrstat, *utxh;

	utrstat = (ioport32_t *) (instance->base + S3C24XX_UTRSTAT);
	utxh = (ioport32_t *) (instance->base + S3C24XX_UTXH);

	/* Wait for transmitter to be empty. */
	while ((pio_read_32(utrstat) & S3C24XX_UTXH_TX_EMPTY) == 0)
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

static outdev_operations_t s3c24xx_uart_ops = {
	.write = s3c24xx_uart_putchar,
	.redraw = NULL
};

outdev_t *s3c24xx_uart_init(ioport8_t *base)
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

/** @}
 */
