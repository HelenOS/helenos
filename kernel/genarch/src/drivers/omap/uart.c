/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 * SPDX-FileCopyrightText: 2013 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Texas Instruments OMAP on-chip uart serial line driver.
 */

#include <assert.h>
#include <genarch/drivers/omap/uart.h>
#include <str.h>
#include <mm/km.h>

static void omap_uart_txb(omap_uart_t *uart, uint8_t b)
{
	/* Wait for buffer */
	while (uart->regs->ssr & OMAP_UART_SSR_TX_FIFO_FULL_FLAG)
		;
	/* Write to the outgoing fifo */
	uart->regs->thr = b;
}

static void omap_uart_putuchar(outdev_t *dev, char32_t ch)
{
	omap_uart_t *uart = dev->data;
	if (!ascii_check(ch)) {
		omap_uart_txb(uart, U_SPECIAL);
	} else {
		if (ch == '\n')
			omap_uart_txb(uart, '\r');
		omap_uart_txb(uart, ch);
	}
}

static outdev_operations_t omap_uart_ops = {
	.write = omap_uart_putuchar,
	.redraw = NULL,
	.scroll_up = NULL,
	.scroll_down = NULL
};

static irq_ownership_t omap_uart_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void omap_uart_handler(irq_t *irq)
{
	omap_uart_t *uart = irq->instance;
	while ((uart->regs->rx_fifo_lvl)) {
		const uint8_t val = uart->regs->rhr;
		if (uart->indev && val) {
			indev_push_character(uart->indev, val);
		}
	}
}

bool omap_uart_init(
    omap_uart_t *uart, inr_t interrupt, uintptr_t addr, size_t size)
{
	assert(uart);
	uart->regs = (void *)km_map(addr, size, KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE);

	assert(uart->regs);

	/* Soft reset the port */
	uart->regs->sysc = OMAP_UART_SYSC_SOFTRESET_FLAG;
	while (!(uart->regs->syss & OMAP_UART_SYSS_RESETDONE_FLAG))
		;

	/* Disable the UART module */
	uart->regs->mdr1 |= OMAP_UART_MDR_MS_DISABLE;

	/* Enable access to EFR register */
	uart->regs->lcr = 0xbf;              /* Sets config mode B */

	/* Enable access to TCL_TLR register */
	const bool enhanced = uart->regs->efr & OMAP_UART_EFR_ENH_FLAG;
	uart->regs->efr |= OMAP_UART_EFR_ENH_FLAG; /* Turn on enh. */
	uart->regs->lcr = 0x80;              /* Config mode A */

	/* Set default (val 0) triggers, disable DMA enable FIFOs */
	const bool tcl_tlr = uart->regs->mcr & OMAP_UART_MCR_TCR_TLR_FLAG;
	/* Enable access to tcr and tlr registers */
	uart->regs->mcr |= OMAP_UART_MCR_TCR_TLR_FLAG;

	/* Enable FIFOs */
	uart->regs->fcr = OMAP_UART_FCR_FIFO_EN_FLAG;

	/*
	 * Enable fine granularity for RX FIFO and set trigger level to 1,
	 * TX FIFO, trigger level is irrelevant
	 */
	uart->regs->lcr = 0xBF;              /* Sets config mode B */
	uart->regs->scr = OMAP_UART_SCR_RX_TRIG_GRANU1_FLAG;
	uart->regs->tlr = 1 << OMAP_UART_TLR_RX_FIFO_TRIG_SHIFT;

	/* Sets config mode A */
	uart->regs->lcr = 0x80;
	/* Restore tcl_tlr access flag */
	if (!tcl_tlr)
		uart->regs->mcr &= ~OMAP_UART_MCR_TCR_TLR_FLAG;
	/* Sets config mode B */
	uart->regs->lcr = 0xBF;

	/* Set the divisor value to get a baud rate of 115200 bps */
	uart->regs->dll = 0x1A;
	uart->regs->dlh = 0x00;

	/* Restore enhanced */
	if (!enhanced)
		uart->regs->efr &= ~OMAP_UART_EFR_ENH_FLAG;

	/* Set the DIV_EN bit to 0 */
	uart->regs->lcr &= ~OMAP_UART_LCR_DIV_EN_FLAG;
	/* Set the BREAK_EN bit to 0 */
	uart->regs->lcr &= ~OMAP_UART_LCR_BREAK_EN_FLAG;
	/* No parity */
	uart->regs->lcr &= ~OMAP_UART_LCR_PARITY_EN_FLAG;
	/* Stop = 1 bit */
	uart->regs->lcr &= ~OMAP_UART_LCR_NB_STOP_FLAG;
	/* Char length = 8 bits */
	uart->regs->lcr |= OMAP_UART_LCR_CHAR_LENGTH_8BITS;

	/* Enable the UART module */
	uart->regs->mdr1 &= (OMAP_UART_MDR_MS_UART16 &
	    ~OMAP_UART_MDR_MS_MASK);

	/* Disable interrupts */
	uart->regs->ier = 0;

	/* Setup outdev */
	outdev_initialize("omap_uart_dev", &uart->outdev, &omap_uart_ops);
	uart->outdev.data = uart;

	/* Initialize IRQ */
	irq_initialize(&uart->irq);
	uart->irq.inr = interrupt;
	uart->irq.claim = omap_uart_claim;
	uart->irq.handler = omap_uart_handler;
	uart->irq.instance = uart;

	return true;
}

void omap_uart_input_wire(omap_uart_t *uart, indev_t *indev)
{
	assert(uart);
	/* Set indev */
	uart->indev = indev;
	/* Register interrupt. */
	irq_register(&uart->irq);
	/* Enable interrupt on receive */
	uart->regs->ier |= OMAP_UART_IER_RHR_IRQ_FLAG;
}

/**
 * @}
 */
