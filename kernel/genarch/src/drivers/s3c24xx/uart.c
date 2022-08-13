/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 *
 * This UART is present on the Samsung S3C24xx CPU (on the gta02 platform).
 */

#include <assert.h>
#include <genarch/drivers/s3c24xx/uart.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/asm.h>
#include <stdlib.h>
#include <mm/page.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

static void s3c24xx_uart_sendb(outdev_t *dev, uint8_t byte)
{
	s3c24xx_uart_t *uart =
	    (s3c24xx_uart_t *) dev->data;

	/* Wait for space becoming available in Tx FIFO. */
	while ((pio_read_32(&uart->io->ufstat) & S3C24XX_UFSTAT_TX_FULL) != 0)
		;

	pio_write_32(&uart->io->utxh, byte);
}

static void s3c24xx_uart_putuchar(outdev_t *dev, char32_t ch)
{
	s3c24xx_uart_t *uart =
	    (s3c24xx_uart_t *) dev->data;

	if ((!uart->parea.mapped) || (console_override)) {
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
	s3c24xx_uart_t *uart = irq->instance;

	while ((pio_read_32(&uart->io->ufstat) & S3C24XX_UFSTAT_RX_COUNT) != 0) {
		uint32_t data = pio_read_32(&uart->io->urxh);
		pio_read_32(&uart->io->uerstat);
		indev_push_character(uart->indev, data & 0xff);
	}
}

static outdev_operations_t s3c24xx_uart_ops = {
	.write = s3c24xx_uart_putuchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

outdev_t *s3c24xx_uart_init(uintptr_t paddr, inr_t inr)
{
	outdev_t *uart_dev = malloc(sizeof(outdev_t));
	if (!uart_dev)
		return NULL;

	s3c24xx_uart_t *uart =
	    malloc(sizeof(s3c24xx_uart_t));
	if (!uart) {
		free(uart_dev);
		return NULL;
	}

	outdev_initialize("s3c24xx_uart_dev", uart_dev, &s3c24xx_uart_ops);
	uart_dev->data = uart;

	uart->io = (s3c24xx_uart_io_t *) km_map(paddr, PAGE_SIZE, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	uart->indev = NULL;

	/* Initialize IRQ structure. */
	irq_initialize(&uart->irq);
	uart->irq.inr = inr;
	uart->irq.claim = s3c24xx_uart_claim;
	uart->irq.handler = s3c24xx_uart_irq_handler;
	uart->irq.instance = uart;

	/* Enable FIFO, Tx trigger level: empty, Rx trigger level: 1 byte. */
	pio_write_32(&uart->io->ufcon, UFCON_FIFO_ENABLE |
	    UFCON_TX_FIFO_TLEVEL_EMPTY | UFCON_RX_FIFO_TLEVEL_1B);

	/* Set RX interrupt to pulse mode */
	pio_write_32(&uart->io->ucon,
	    pio_read_32(&uart->io->ucon) & ~UCON_RX_INT_LEVEL);

	ddi_parea_init(&uart->parea);
	uart->parea.pbase = paddr;
	uart->parea.frames = 1;
	uart->parea.unpriv = false;
	uart->parea.mapped = false;
	ddi_parea_register(&uart->parea);

	if (!fb_exported) {
		/*
		 * This is the necessary evil until
		 * the userspace driver is entirely
		 * self-sufficient.
		 */
		sysinfo_set_item_val("fb", NULL, true);
		sysinfo_set_item_val("fb.kind", NULL, 3);
		sysinfo_set_item_val("fb.address.physical", NULL, paddr);

		fb_exported = true;
	}

	return uart_dev;
}

void s3c24xx_uart_input_wire(s3c24xx_uart_t *uart, indev_t *indev)
{
	assert(uart);
	assert(indev);

	uart->indev = indev;
	irq_register(&uart->irq);
}

/** @}
 */
