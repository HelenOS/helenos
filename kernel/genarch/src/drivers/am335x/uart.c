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
 * @brief Texas Instruments AM335x on-chip uart serial line driver.
 */

#include <genarch/drivers/am335x/uart.h>
#include <ddi/device.h>
#include <str.h>
#include <mm/km.h>

static void am335x_uart_txb(am335x_uart_t *uart, uint8_t b)
{
	/* Wait for buffer */
	while (uart->regs->ssr & AM335x_UART_SSR_TX_FIFO_FULL_FLAG);
	/* Write to the outgoing fifo */
	uart->regs->thr = b;
}

static void am335x_uart_putchar(outdev_t *dev, wchar_t ch)
{
	am335x_uart_t *uart = dev->data;
	if (!ascii_check(ch)) {
		am335x_uart_txb(uart, U_SPECIAL);
	} else {
		if (ch == '\n')
			am335x_uart_txb(uart, '\r');
		am335x_uart_txb(uart, ch);
	}
}

static outdev_operations_t am335x_uart_ops = {
	.redraw = NULL,
	.write = am335x_uart_putchar,
};

static irq_ownership_t am335x_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void am335x_uart_handler(irq_t *irq)
{
	am335x_uart_t *uart = irq->instance;
	while ((uart->regs->rx_fifo_lvl)) {
		const uint8_t val = uart->regs->rhr;
		if (uart->indev && val) {
			indev_push_character(uart->indev, val);
		}
	}
}

bool am335x_uart_init(
    am335x_uart_t *uart, inr_t interrupt, uintptr_t addr, size_t size)
{
	ASSERT(uart);
	uart->regs = (void *)km_map(addr, size, PAGE_NOT_CACHEABLE);

	ASSERT(uart->regs);

	/* See TI OMAP35X TRM ch 17.5.1.1 p. 2732 for startup routine */
#if 0
	/* Soft reset the port */
	uart->regs->sysc = AM335x_UART_SYSC_SOFTRESET_FLAG;
	while (!(uart->regs->syss & AM335x_UART_SYSS_RESETDONE_FLAG)) ;
#endif

	/* Enable access to EFR register */
	const uint8_t lcr = uart->regs->lcr; /* Save old value */
	uart->regs->lcr = 0xbf;              /* Sets config mode B */

	/* Enable access to TCL_TLR register */
	const bool enhanced = uart->regs->efr & AM335x_UART_EFR_ENH_FLAG;
	uart->regs->efr |= AM335x_UART_EFR_ENH_FLAG; /* Turn on enh. */
	uart->regs->lcr = 0x80;              /* Config mode A */

	/* Set default (val 0) triggers, disable DMA enable FIFOs */
	const bool tcl_tlr = uart->regs->mcr & AM335x_UART_MCR_TCR_TLR_FLAG;
	/* Enable access to tcr and tlr registers */
	uart->regs->mcr |= AM335x_UART_MCR_TCR_TLR_FLAG;

	/* Enable FIFOs */
	uart->regs->fcr = AM335x_UART_FCR_FIFO_EN_FLAG;

	/* Eneble fine granularity for RX FIFO and set trigger level to 1,
	 * TX FIFO, trigger level is irelevant*/
	uart->regs->lcr = 0xbf;              /* Sets config mode B */
	uart->regs->scr = AM335x_UART_SCR_RX_TRIG_GRANU1_FLAG;
	uart->regs->tlr = 1 << AM335x_UART_TLR_RX_FIFO_TRIG_SHIFT;

	/* Restore enhanced */
	if (!enhanced)
		uart->regs->efr &= ~AM335x_UART_EFR_ENH_FLAG;

	uart->regs->lcr = 0x80;              /* Config mode A */
	/* Restore tcl_lcr access flag*/
	if (!tcl_tlr)
		uart->regs->mcr &= ~AM335x_UART_MCR_TCR_TLR_FLAG;

	/* Restore lcr */
	uart->regs->lcr = lcr;

	/* Disable interrupts */
	uart->regs->ier = 0;

	/* Setup outdev */
	outdev_initialize("am335x_uart_dev", &uart->outdev, &am335x_uart_ops);
	uart->outdev.data = uart;

	/* Initialize IRQ */
	irq_initialize(&uart->irq);
	uart->irq.devno = device_assign_devno();
	uart->irq.inr = interrupt;
	uart->irq.claim = am335x_uart_claim;
	uart->irq.handler = am335x_uart_handler;
	uart->irq.instance = uart;

	return true;
}

void am335x_uart_input_wire(am335x_uart_t *uart, indev_t *indev)
{
	ASSERT(uart);
	/* Set indev */
	uart->indev = indev;
	/* Register interrupt. */
	irq_register(&uart->irq);
	/* Enable interrupt on receive */
	uart->regs->ier |= AM335x_UART_IER_RHR_IRQ_FLAG;
}

/**
 * @}
 */
