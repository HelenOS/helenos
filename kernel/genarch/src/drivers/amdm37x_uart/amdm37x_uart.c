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
 * @brief Texas Instruments AMDM37x on-chip uart serial line driver.
 */

#include <genarch/drivers/amdm37x_uart/amdm37x_uart.h>
#include <ddi/device.h>
#include <str.h>
#include <mm/km.h>

static void amdm37x_uart_txb(amdm37x_uart_t *uart, uint8_t b)
{
	/* Wait for buffer */
	while (uart->regs->ssr & AMDM37x_UART_SSR_TX_FIFO_FULL_FLAG);
	/* Write to the outgoing fifo */
	uart->regs->thr = b;
}

static void amdm37x_uart_putchar(outdev_t *dev, wchar_t ch)
{
	amdm37x_uart_t *uart = dev->data;
	if (!ascii_check(ch)) {
		amdm37x_uart_txb(uart, U_SPECIAL);
	} else {
		if (ch == '\n')
			amdm37x_uart_txb(uart, '\r');
		amdm37x_uart_txb(uart, ch);
	}
}

static outdev_operations_t amdm37x_uart_ops = {
	.redraw = NULL,
	.write = amdm37x_uart_putchar,
};

static irq_ownership_t amdm37x_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void amdm37x_uart_handler(irq_t *irq)
{
	amdm37x_uart_t *uart = irq->instance;
//TODO enable while checking when RX FIFO is used instead of single char.
//	while (!(uart->regs->isr2 & AMDM37x_UART_ISR2_RX_FIFO_EMPTY_FLAG)) {
		const uint8_t val = uart->regs->rhr;
		if (uart->indev)
			indev_push_character(uart->indev, val);
//	}
}

bool amdm37x_uart_init(
    amdm37x_uart_t *uart, inr_t interrupt, uintptr_t addr, size_t size)
{
	ASSERT(uart);
	uart->regs = (void *)km_map(addr, size, PAGE_NOT_CACHEABLE);

	ASSERT(uart->regs);

	/* See TI OMAP35X TRM ch 17.5.1.1 p. 2732 for startup routine */
#if 0
	/* Soft reset the port */
	uart->regs->sysc = AMDM37x_UART_SYSC_SOFTRESET_FLAG;
	while (uart->regs->syss & AMDM37x_UART_SYSS_RESETDONE_FLAG) ;

	/* Enable access to EFR register */
	const uint8_t lcr = uart->regs->lcr; /* Save old value */
	uart->regs->lcr = 0xbf;              /* Sets config mode B */

	/* Enable access to TCL_TLR register */
	const bool enhanced = uart->regs->efr & AMDM37x_UART_EFR_ENH_FLAG;
	uart->regs->efr |= AMDM37x_UART_EFR_ENH_FLAG; /* Turn on enh. */
	uart->regs->lcr = 0x80;              /* Config mode A */

	/* Set default (val 0) triggers, disable DMA enable FIFOs */
	const bool tcl_tlr = uart->regs->mcr & AMDM37x_UART_MCR_TCR_TLR_FLAG;
	uart->regs->fcr = AMDM37x_UART_FCR_FIFO_EN_FLAG;

	/* Enable fine granularity for rx trigger */
	uart->regs->lcr = 0xbf;              /* Sets config mode B */
	uart->regs->scr = AMDM37x_UART_SCR_RX_TRIG_GRANU1_FLAG;

	/* Restore enhanced */
	if (!enhanced)
		uart->regs->efr &= ~AMDM37x_UART_EFR_ENH_FLAG;

	uart->regs->lcr = 0x80;              /* Config mode A */
	/* Restore tcl_lcr */
	if (!tcl_tlr)
		uart->regs->mcr &= ~AMDM37x_UART_MCR_TCR_TLR_FLAG;

	/* Restore tcl_lcr */
	uart->regs->lcr = lcr;

	/* Disable interrupts */
	uart->regs->ier = 0;
#endif
	/* Setup outdev */
	outdev_initialize("amdm37x_uart_dev", &uart->outdev, &amdm37x_uart_ops);
	uart->outdev.data = uart;

	/* Initialize IRQ */
	irq_initialize(&uart->irq);
	uart->irq.devno = device_assign_devno();
	uart->irq.inr = interrupt;
	uart->irq.claim = amdm37x_uart_claim;
	uart->irq.handler = amdm37x_uart_handler;
	uart->irq.instance = uart;
	irq_register(&uart->irq);

	return true;
}

void amdm37x_uart_input_wire(amdm37x_uart_t *uart, indev_t *indev)
{
	ASSERT(uart);
	/* Set indev */
	uart->indev = indev;
	/* Enable interrupt on receive */
	uart->regs->ier |= AMDM37x_UART_IER_RHR_IRQ_FLAG;

	// TODO set rx fifo
	// TODO set rx fifo threshold to 1
}

/**
 * @}
 */
