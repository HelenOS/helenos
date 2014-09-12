/*
 * Copyright (c) 2009 Martin Decky
 * Copyright (c) 2010 Jiri Svoboda
 * Copyright (c) 2013 Jakub Klama
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
 * @brief Gaisler GRLIB UART IP-Core driver.
 */

#include <genarch/drivers/grlib/uart.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/device.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

static void grlib_uart_sendb(outdev_t *dev, uint8_t byte)
{
	grlib_uart_status_t *status;
	grlib_uart_t *uart = (grlib_uart_t *) dev->data;
	
	/* Wait for space becoming available in Tx FIFO. */
	do {
		uint32_t reg = pio_read_32(&uart->io->status);
		status = (grlib_uart_status_t *) &reg;
	} while (status->tf != 0);
	
	pio_write_32(&uart->io->data, byte);
}

static void grlib_uart_putchar(outdev_t *dev, wchar_t ch)
{
	grlib_uart_t *uart = (grlib_uart_t *) dev->data;
	
	if ((!uart->parea.mapped) || (console_override)) {
		if (!ascii_check(ch)) {
			grlib_uart_sendb(dev, U_SPECIAL);
		} else {
			if (ch == '\n')
				grlib_uart_sendb(dev, (uint8_t) '\r');
			
			grlib_uart_sendb(dev, (uint8_t) ch);
		}
	}
}

static irq_ownership_t grlib_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void grlib_uart_irq_handler(irq_t *irq)
{
	grlib_uart_t *uart = irq->instance;
	
	uint32_t reg = pio_read_32(&uart->io->status);
	grlib_uart_status_t *status = (grlib_uart_status_t *) &reg;
	
	while (status->dr != 0) {
		uint32_t data = pio_read_32(&uart->io->data);
		reg = pio_read_32(&uart->io->status);
		status = (grlib_uart_status_t *) &reg;
		indev_push_character(uart->indev, data & 0xff);
	}
}

static outdev_operations_t grlib_uart_ops = {
	.write = grlib_uart_putchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

outdev_t *grlib_uart_init(uintptr_t paddr, inr_t inr)
{
	outdev_t *uart_dev = malloc(sizeof(outdev_t), FRAME_ATOMIC);
	if (!uart_dev)
		return NULL;
	
	grlib_uart_t *uart = malloc(sizeof(grlib_uart_t), FRAME_ATOMIC);
	if (!uart) {
		free(uart_dev);
		return NULL;
	}
	
	outdev_initialize("grlib_uart_dev", uart_dev, &grlib_uart_ops);
	uart_dev->data = uart;
	
	uart->io = (grlib_uart_io_t *) km_map(paddr, PAGE_SIZE,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	uart->indev = NULL;
	
	/* Initialize IRQ structure. */
	irq_initialize(&uart->irq);
	uart->irq.devno = device_assign_devno();
	uart->irq.inr = inr;
	uart->irq.claim = grlib_uart_claim;
	uart->irq.handler = grlib_uart_irq_handler;
	uart->irq.instance = uart;
	
	/* Enable FIFO, Tx trigger level: empty, Rx trigger level: 1 byte. */
	grlib_uart_control_t control = {
		.fa = 1,
		.rf = 1,
		.tf = 1,
		.ri = 1,
		.te = 1,
		.re = 1
	};
	
	uint32_t *reg = (uint32_t *) &control;
	pio_write_32(&uart->io->control, *reg);
	
	link_initialize(&uart->parea.link);
	uart->parea.pbase = paddr;
	uart->parea.frames = 1;
	uart->parea.unpriv = false;
	uart->parea.mapped = false;
	ddi_parea_register(&uart->parea);
	
	return uart_dev;
}

void grlib_uart_input_wire(grlib_uart_t *uart, indev_t *indev)
{
	ASSERT(uart);
	ASSERT(indev);
	
	uart->indev = indev;
	irq_register(&uart->irq);
}

/** @}
 */
