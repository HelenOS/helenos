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
 * @brief ARM926 on-chip UART (PrimeCell UART, PL011) driver.
 */

#include <genarch/drivers/arm926_uart/arm926_uart.h>
#include <console/chardev.h>
#include <console/console.h>
#include <ddi/device.h>
#include <arch/asm.h>
#include <mm/slab.h>
#include <mm/page.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>
#include <str.h>

static void arm926_uart_sendb(arm926_uart_t *uart, uint8_t byte)
{
	/* Wait for space becoming available in Tx FIFO. */
	// TODO make pio_read accept consts pointers and remove the cast
	while ((pio_read_32((ioport32_t*)&uart->regs->flag) & ARM926_UART_FLAG_TXFF_FLAG) != 0)
		;

	pio_write_32(&uart->regs->data, byte);
}

static void arm926_uart_putchar(outdev_t *dev, wchar_t ch)
{
	arm926_uart_t *uart = dev->data;

	if (!ascii_check(ch)) {
		arm926_uart_sendb(uart, U_SPECIAL);
	} else {
		if (ch == '\n')
			arm926_uart_sendb(uart, (uint8_t) '\r');
		arm926_uart_sendb(uart, (uint8_t) ch);
	}
}

static outdev_operations_t arm926_uart_ops = {
	.write = arm926_uart_putchar,
	.redraw = NULL,
};

static irq_ownership_t arm926_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void arm926_uart_irq_handler(irq_t *irq)
{
	arm926_uart_t *uart = irq->instance;

	// TODO make pio_read accept consts pointers and remove the cast
	while ((pio_read_32((ioport32_t*)&uart->regs->flag) & ARM926_UART_FLAG_RXFE_FLAG) == 0) {
		/* We ignore all error flags here */
		const uint8_t data = pio_read_32(&uart->regs->data);
		if (uart->indev)
			indev_push_character(uart->indev, data);
	}
	/* Ack interrupts */
	pio_write_32(&uart->regs->interrupt_clear, ARM926_UART_INTERRUPT_ALL);
}

bool arm926_uart_init(
    arm926_uart_t *uart, inr_t interrupt, uintptr_t addr, size_t size)
{
	ASSERT(uart);
	uart->regs = (void*)km_map(addr, size, PAGE_NOT_CACHEABLE);

	ASSERT(uart->regs);

	/* Enable hw flow control */
	uart->regs->control = 0 |
	    ARM926_UART_CONTROL_UARTEN_FLAG |
	    ARM926_UART_CONTROL_RTSE_FLAG |
	    ARM926_UART_CONTROL_CTSE_FLAG;

	/* Mask all interrupts */
	uart->regs->interrupt_mask = ARM926_UART_INTERRUPT_ALL;

	outdev_initialize("arm926_uart_dev", &uart->outdev, &arm926_uart_ops);
	uart->outdev.data = uart;

	/* Initialize IRQ */
	irq_initialize(&uart->irq);
        uart->irq.devno = device_assign_devno();
        uart->irq.inr = interrupt;
        uart->irq.claim = arm926_uart_claim;
        uart->irq.handler = arm926_uart_irq_handler;
        uart->irq.instance = uart;

	return true;
}

void arm926_uart_input_wire(arm926_uart_t *uart, indev_t *indev)
{
	ASSERT(uart);
	ASSERT(indev);

	uart->indev = indev;
	irq_register(&uart->irq);
	/* Enable receive interrupt */
	uart->regs->interrupt_mask &= ~ARM926_UART_INTERRUPT_RX_FLAG;
}

/** @}
 */

