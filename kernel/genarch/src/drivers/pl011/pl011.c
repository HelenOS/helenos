/*
 * Copyright (c) 2012 Jan Vesely
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
 * @brief ARM PrimeCell PL011 UART driver.
 */

#include <assert.h>
#include <genarch/drivers/pl011/pl011.h>
#include <console/chardev.h>
#include <console/console.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

static void pl011_uart_sendb(pl011_uart_t *uart, uint8_t byte)
{
	/* Wait for space becoming available in Tx FIFO. */
	// TODO make pio_read accept consts pointers and remove the cast
	while ((pio_read_32((ioport32_t *)&uart->regs->flag) & PL011_UART_FLAG_TXFF_FLAG) != 0)
		;

	pio_write_32(&uart->regs->data, byte);
}

static void pl011_uart_putchar(outdev_t *dev, wchar_t ch)
{
	pl011_uart_t *uart = dev->data;

	if (!ascii_check(ch)) {
		pl011_uart_sendb(uart, U_SPECIAL);
	} else {
		if (ch == '\n')
			pl011_uart_sendb(uart, (uint8_t) '\r');
		pl011_uart_sendb(uart, (uint8_t) ch);
	}
}

static outdev_operations_t pl011_uart_ops = {
	.write = pl011_uart_putchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

static irq_ownership_t pl011_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void pl011_uart_irq_handler(irq_t *irq)
{
	pl011_uart_t *uart = irq->instance;

	// TODO make pio_read accept const pointers and remove the cast
	while ((pio_read_32((ioport32_t *)&uart->regs->flag) & PL011_UART_FLAG_RXFE_FLAG) == 0) {
		/* We ignore all error flags here */
		const uint8_t data = pio_read_32(&uart->regs->data);
		if (uart->indev)
			indev_push_character(uart->indev, data);
	}
	/* Ack interrupts */
	pio_write_32(&uart->regs->interrupt_clear, PL011_UART_INTERRUPT_ALL);
}

bool pl011_uart_init(pl011_uart_t *uart, inr_t interrupt, uintptr_t addr)
{
	assert(uart);
	uart->regs = (void *)km_map(addr, sizeof(pl011_uart_regs_t),
	    PAGE_NOT_CACHEABLE);
	assert(uart->regs);

	/* Disable UART */
	uart->regs->control &= ~PL011_UART_CONTROL_UARTEN_FLAG;

	/* Enable hw flow control */
	uart->regs->control |=
	    PL011_UART_CONTROL_RTSE_FLAG |
	    PL011_UART_CONTROL_CTSE_FLAG;

	/* Mask all interrupts */
	uart->regs->interrupt_mask = 0;
	/* Clear interrupts */
	uart->regs->interrupt_clear = PL011_UART_INTERRUPT_ALL;
	/* Enable UART, TX and RX */
	uart->regs->control |=
	    PL011_UART_CONTROL_UARTEN_FLAG |
	    PL011_UART_CONTROL_TXE_FLAG |
	    PL011_UART_CONTROL_RXE_FLAG;

	outdev_initialize("pl011_uart_dev", &uart->outdev, &pl011_uart_ops);
	uart->outdev.data = uart;

	/* Initialize IRQ */
	irq_initialize(&uart->irq);
	uart->irq.inr = interrupt;
	uart->irq.claim = pl011_uart_claim;
	uart->irq.handler = pl011_uart_irq_handler;
	uart->irq.instance = uart;

	return true;
}

void pl011_uart_input_wire(pl011_uart_t *uart, indev_t *indev)
{
	assert(uart);
	assert(indev);

	uart->indev = indev;
	irq_register(&uart->irq);
	/* Enable receive interrupts */
	uart->regs->interrupt_mask |=
	    PL011_UART_INTERRUPT_RX_FLAG |
	    PL011_UART_INTERRUPT_RT_FLAG;
}

/** @}
 */

