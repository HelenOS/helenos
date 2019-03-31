/*
 * Copyright (c) 2016 Petr Pavlu
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

/** @file ARM PrimeCell PL011 UART driver.
 */

#include <async.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <io/chardev_srv.h>
#include <macros.h>

#include "pl011.h"

/** PL011 register map. */
typedef struct {
	/** UART data register. */
	ioport32_t data;
	union {
		/** Receive status register (same values that are in upper bits
		 * of data register).
		 */
		const ioport32_t status;
		/** Error clear register (writing anything clears all errors).
		 */
		ioport32_t error_clear;
	};
	/** Reserved. */
	PADD32(4);
	/** Flag register. */
	const ioport32_t flag;
	/** Transmit FIFO full. */
#define PL011_UART_FLAG_TXFF_FLAG  (1 << 5)

	/** Reserved. */
	PADD32(1);
	/** IrDA low-power counter register. */
	ioport32_t irda_low_power;
	/** Integer baud rate register. */
	ioport32_t int_baud_divisor;
	/** Fractional baud rate register. */
	ioport32_t fract_baud_divisor;
	/** Line control register. */
	ioport32_t line_control_high;
	/** Control register. */
	ioport32_t control;
	/** Interrupt FIFO level select register. */
	ioport32_t interrupt_fifo;
	/** Interrupt mask set/clear register. */
	ioport32_t interrupt_mask;
	/** Raw interrupt status register (pending interrupts before applying
	 * the mask).
	 */
	const ioport32_t raw_interrupt_status;
	/** Masked interrupt status register (pending interrupts after applying
	 * the mask).
	 */
	const ioport32_t masked_interrupt_status;
	/** Interrupt clear register (write 1s to clear pending interrupts). */
	ioport32_t interrupt_clear;

	/** Interrupt indicating a change in the nUARTRI modem status. */
#define PL011_UART_INTERRUPT_RIM_FLAG   (1 << 0)
	/** Interrupt indicating a change in the nUARTCTS modem status. */
#define PL011_UART_INTERRUPT_CTSM_FLAG  (1 << 1)
	/** Interrupt indicating a change in the nUARTDCD modem status. */
#define PL011_UART_INTERRUPT_DCDM_FLAG  (1 << 2)
	/** Interrupt indicating a change in the nUARTDSR modem status. */
#define PL011_UART_INTERRUPT_DSRM_FLAG  (1 << 3)
	/** The receive interrupt. */
#define PL011_UART_INTERRUPT_RX_FLAG    (1 << 4)
	/** The transmit interrupt. */
#define PL011_UART_INTERRUPT_TX_FLAG    (1 << 5)
	/** The receive timeout interrupt.  */
#define PL011_UART_INTERRUPT_RT_FLAG    (1 << 6)
	/** Interrupt indicating an overrun error. */
#define PL011_UART_INTERRUPT_FE_FLAG    (1 << 7)
	/** Interrupt indicating a break in the reception. */
#define PL011_UART_INTERRUPT_PE_FLAG    (1 << 8)
	/** Interrupt indicating a parity error in the received character. */
#define PL011_UART_INTERRUPT_BE_FLAG    (1 << 9)
	/** Interrupt indicating a framing error in the received character. */
#define PL011_UART_INTERRUPT_OE_FLAG    (1 << 10)
	/** All interrupt mask. */
#define PL011_UART_INTERRUPT_ALL        0x3ff

	/** DMA control register. */
	ioport32_t dma_control;
	/** Reserved. */
	PADD32(13);
	/** Reserved for test purposes. */
	PADD32(4);
	/** Reserved. */
	PADD32(976);
	/** Reserved for future ID expansion. */
	PADD32(4);
	/** UARTPeriphID0 register. */
	const ioport32_t periph_id0;
	/** UARTPeriphID1 register. */
	const ioport32_t periph_id1;
	/** UARTPeriphID2 register. */
	const ioport32_t periph_id2;
	/** UARTPeriphID3 register. */
	const ioport32_t periph_id3;
	/** UARTPCellID0 register. */
	const ioport32_t cell_id0;
	/** UARTPCellID1 register. */
	const ioport32_t cell_id1;
	/** UARTPCellID2 register. */
	const ioport32_t cell_id2;
	/** UARTPCellID3 register. */
	const ioport32_t cell_id3;
} pl011_uart_regs_t;

static void pl011_connection(ipc_call_t *, void *);

static errno_t pl011_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
static errno_t pl011_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t pl011_chardev_ops = {
	.read = pl011_read,
	.write = pl011_write
};

/** Address range accessed by the PL011 interrupt pseudo-code. */
static const irq_pio_range_t pl011_ranges_proto[] = {
	{
		.base = 0,
		.size = sizeof(pl011_uart_regs_t)
	}
};

/** PL011 interrupt pseudo-code instructions. */
static const irq_cmd_t pl011_cmds_proto[] = {
	{
		/* Read masked_interrupt_status. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL,
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.value = PL011_UART_INTERRUPT_RX_FLAG |
		    PL011_UART_INTERRUPT_RT_FLAG,
		.srcarg = 1,
		.dstarg = 3
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 1,
		.srcarg = 3
	},
	{
		/* Read data. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL,
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Process an interrupt from a PL011 device. */
static void pl011_irq_handler(ipc_call_t *call, void *arg)
{
	pl011_t *pl011 = (pl011_t *) arg;
	uint32_t intrs = ipc_get_arg1(call);
	uint8_t c = ipc_get_arg2(call);
	errno_t rc;

	if ((intrs & (PL011_UART_INTERRUPT_RX_FLAG |
	    PL011_UART_INTERRUPT_RT_FLAG)) == 0) {
		/* TODO */
		return;
	}

	fibril_mutex_lock(&pl011->buf_lock);

	rc = circ_buf_push(&pl011->cbuf, &c);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Buffer overrun");

	fibril_mutex_unlock(&pl011->buf_lock);
	fibril_condvar_broadcast(&pl011->buf_cv);
}

/** Add a PL011 device. */
errno_t pl011_add(pl011_t *pl011, pl011_res_t *res)
{
	ddf_fun_t *fun = NULL;
	irq_pio_range_t *pl011_ranges = NULL;
	irq_cmd_t *pl011_cmds = NULL;
	errno_t rc;

	circ_buf_init(&pl011->cbuf, pl011->buf, pl011_buf_size, 1);
	fibril_mutex_initialize(&pl011->buf_lock);
	fibril_condvar_initialize(&pl011->buf_cv);

	pl011->irq_handle = CAP_NIL;

	pl011_ranges = malloc(sizeof(pl011_ranges_proto));
	if (pl011_ranges == NULL) {
		rc = ENOMEM;
		goto error;
	}
	pl011_cmds = malloc(sizeof(pl011_cmds_proto));
	if (pl011_cmds == NULL) {
		rc = ENOMEM;
		goto error;
	}

	pl011->res = *res;

	fun = ddf_fun_create(pl011->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	rc = pio_enable(
	    (void *) res->base, sizeof(pl011_uart_regs_t), &pl011->regs);
	if (rc != EOK)
		goto error;

	ddf_fun_set_conn_handler(fun, pl011_connection);

	memcpy(pl011_ranges, pl011_ranges_proto, sizeof(pl011_ranges_proto));
	memcpy(pl011_cmds, pl011_cmds_proto, sizeof(pl011_cmds_proto));
	pl011_ranges[0].base = res->base;
	pl011_uart_regs_t *regsphys = (pl011_uart_regs_t *) res->base;
	pl011_cmds[0].addr = (void *) &regsphys->masked_interrupt_status;
	pl011_cmds[3].addr = (void *) &regsphys->data;

	pl011->irq_code.rangecount =
	    sizeof(pl011_ranges_proto) / sizeof(irq_pio_range_t);
	pl011->irq_code.ranges = pl011_ranges;
	pl011->irq_code.cmdcount = sizeof(pl011_cmds_proto) / sizeof(irq_cmd_t);
	pl011->irq_code.cmds = pl011_cmds;

	rc = async_irq_subscribe(res->irq, pl011_irq_handler, pl011,
	    &pl011->irq_code, &pl011->irq_handle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ code.");
		goto error;
	}

	chardev_srvs_init(&pl011->cds);
	pl011->cds.ops = &pl011_chardev_ops;
	pl011->cds.sarg = pl011;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	ddf_fun_add_to_category(fun, "console");

	return EOK;
error:
	if (cap_handle_valid(pl011->irq_handle))
		async_irq_unsubscribe(pl011->irq_handle);
	if (fun != NULL)
		ddf_fun_destroy(fun);
	free(pl011_ranges);
	free(pl011_cmds);

	return rc;
}

/** Remove a PL011 device. */
errno_t pl011_remove(pl011_t *pl011)
{
	return ENOTSUP;
}

/** A PL011 device gone. */
errno_t pl011_gone(pl011_t *pl011)
{
	return ENOTSUP;
}

/** Send a character to a PL011 device.
 *
 * @param c Character to be printed.
 */
static void pl011_putchar(pl011_t *pl011, uint8_t ch)
{
	pl011_uart_regs_t *regs = (pl011_uart_regs_t *) pl011->regs;

	/* Wait for space to become available in the TX FIFO. */
	while (pio_read_32(&regs->flag) & PL011_UART_FLAG_TXFF_FLAG)
		;

	pio_write_32(&regs->data, ch);
}

/** Read from a PL011 device. */
static errno_t pl011_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	pl011_t *pl011 = (pl011_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	fibril_mutex_lock(&pl011->buf_lock);

	while ((flags & chardev_f_nonblock) == 0 &&
	    circ_buf_nused(&pl011->cbuf) == 0)
		fibril_condvar_wait(&pl011->buf_cv, &pl011->buf_lock);

	p = 0;
	while (p < size) {
		rc = circ_buf_pop(&pl011->cbuf, &bp[p]);
		if (rc != EOK)
			break;
		++p;
	}

	fibril_mutex_unlock(&pl011->buf_lock);

	*nread = p;
	return EOK;
}

/** Write to a PL011 device. */
static errno_t pl011_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	pl011_t *pl011 = (pl011_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		pl011_putchar(pl011, dp[i]);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void pl011_connection(ipc_call_t *icall, void *arg)
{
	pl011_t *pl011 = (pl011_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &pl011->cds);
}

/** @}
 */
