/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
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

static void pl011_uart_putuchar(outdev_t *dev, char32_t ch)
{
	pl011_uart_t *uart = dev->data;

	/* If the userspace owns the console, do not output anything. */
	if (uart->parea.mapped && !console_override)
		return;

	if (!ascii_check(ch))
		pl011_uart_sendb(uart, U_SPECIAL);
	else {
		if (ch == '\n')
			pl011_uart_sendb(uart, (uint8_t) '\r');
		pl011_uart_sendb(uart, (uint8_t) ch);
	}
}

static outdev_operations_t pl011_uart_ops = {
	.write = pl011_uart_putuchar,
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
	    KM_NATURAL_ALIGNMENT, PAGE_WRITE | PAGE_NOT_CACHEABLE);
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

	ddi_parea_init(&uart->parea);
	uart->parea.pbase = addr;
	uart->parea.frames = 1;
	uart->parea.unpriv = false;
	uart->parea.mapped = false;
	ddi_parea_register(&uart->parea);

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
