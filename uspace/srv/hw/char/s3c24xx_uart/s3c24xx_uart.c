/*
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

/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 *
 * This UART is present on the Samsung S3C24xx CPU (on the gta02 platform).
 */

#include <async.h>
#include <ddi.h>
#include <errno.h>
#include <inttypes.h>
#include <io/chardev_srv.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysinfo.h>
#include "s3c24xx_uart.h"

#define NAME       "s3c24ser"
#define NAMESPACE  "char"

static irq_cmd_t uart_irq_cmds[] = {
	{
		.cmd = CMD_ACCEPT
	}
};

static irq_code_t uart_irq_code = {
	0,
	NULL,
	sizeof(uart_irq_cmds) / sizeof(irq_cmd_t),
	uart_irq_cmds
};

/** S3C24xx UART instance structure */
static s3c24xx_uart_t *uart;

static void s3c24xx_uart_connection(cap_call_handle_t, ipc_call_t *, void *);
static void s3c24xx_uart_irq_handler(ipc_call_t *, void *);
static int s3c24xx_uart_init(s3c24xx_uart_t *);
static void s3c24xx_uart_sendb(s3c24xx_uart_t *, uint8_t);

static errno_t s3c24xx_uart_read(chardev_srv_t *, void *, size_t, size_t *);
static errno_t s3c24xx_uart_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t s3c24xx_uart_chardev_ops = {
	.read = s3c24xx_uart_read,
	.write = s3c24xx_uart_write
};

int main(int argc, char *argv[])
{
	printf("%s: S3C24xx on-chip UART driver\n", NAME);

	async_set_fallback_port_handler(s3c24xx_uart_connection, uart);
	errno_t rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register server.\n", NAME);
		return rc;
	}

	uart = malloc(sizeof(s3c24xx_uart_t));
	if (uart == NULL)
		return -1;

	if (s3c24xx_uart_init(uart) != EOK)
		return -1;

	rc = loc_service_register(NAMESPACE "/" NAME, &uart->service_id);
	if (rc != EOK) {
		printf(NAME ": Unable to register device %s.\n",
		    NAMESPACE "/" NAME);
		return -1;
	}

	printf(NAME ": Registered device %s.\n", NAMESPACE "/" NAME);

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

/** Character device connection handler. */
static void s3c24xx_uart_connection(cap_call_handle_t icall_handle, ipc_call_t *icall,
    void *arg)
{
	s3c24xx_uart_t *uart = (s3c24xx_uart_t *) arg;

	chardev_conn(icall_handle, icall, &uart->cds);
}


static void s3c24xx_uart_irq_handler(ipc_call_t *call, void *arg)
{
	errno_t rc;

	(void) call;
	(void) arg;

	while ((pio_read_32(&uart->io->ufstat) & S3C24XX_UFSTAT_RX_COUNT) != 0) {
		uint32_t data = pio_read_32(&uart->io->urxh) & 0xff;
		uint32_t status = pio_read_32(&uart->io->uerstat);

		fibril_mutex_lock(&uart->buf_lock);

		rc = circ_buf_push(&uart->cbuf, &data);
		if (rc != EOK)
			printf(NAME ": Buffer overrun\n");

		fibril_mutex_unlock(&uart->buf_lock);
		fibril_condvar_broadcast(&uart->buf_cv);

		if (status != 0)
			printf(NAME ": Error status 0x%x\n", status);
	}
}

/** Initialize S3C24xx on-chip UART. */
static int s3c24xx_uart_init(s3c24xx_uart_t *uart)
{
	void *vaddr;
	sysarg_t inr;

	circ_buf_init(&uart->cbuf, uart->buf, s3c24xx_uart_buf_size, 1);
	fibril_mutex_initialize(&uart->buf_lock);
	fibril_condvar_initialize(&uart->buf_cv);

	if (sysinfo_get_value("s3c24xx_uart.address.physical",
	    &uart->paddr) != EOK)
		return -1;

	if (pio_enable((void *) uart->paddr, sizeof(s3c24xx_uart_io_t),
	    &vaddr) != 0)
		return -1;

	if (sysinfo_get_value("s3c24xx_uart.inr", &inr) != EOK)
		return -1;

	uart->io = vaddr;

	printf(NAME ": device at physical address %p, inr %" PRIun ".\n",
	    (void *) uart->paddr, inr);

	async_irq_subscribe(inr, s3c24xx_uart_irq_handler, NULL, &uart_irq_code, NULL);

	/* Enable FIFO, Tx trigger level: empty, Rx trigger level: 1 byte. */
	pio_write_32(&uart->io->ufcon, UFCON_FIFO_ENABLE |
	    UFCON_TX_FIFO_TLEVEL_EMPTY | UFCON_RX_FIFO_TLEVEL_1B);

	/* Set RX interrupt to pulse mode */
	pio_write_32(&uart->io->ucon,
	    pio_read_32(&uart->io->ucon) & ~UCON_RX_INT_LEVEL);

	chardev_srvs_init(&uart->cds);
	uart->cds.ops = &s3c24xx_uart_chardev_ops;
	uart->cds.sarg = uart;

	return EOK;
}

/** Send a byte to the UART. */
static void s3c24xx_uart_sendb(s3c24xx_uart_t *uart, uint8_t byte)
{
	/* Wait for space becoming available in Tx FIFO. */
	while ((pio_read_32(&uart->io->ufstat) & S3C24XX_UFSTAT_TX_FULL) != 0)
		;

	pio_write_32(&uart->io->utxh, byte);
}

static errno_t s3c24xx_uart_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread)
{
	s3c24xx_uart_t *uart = (s3c24xx_uart_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	fibril_mutex_lock(&uart->buf_lock);

	while (circ_buf_nused(&uart->cbuf) == 0)
		fibril_condvar_wait(&uart->buf_cv, &uart->buf_lock);

	p = 0;
	while (p < size) {
		rc = circ_buf_pop(&uart->cbuf, &bp[p]);
		if (rc != EOK)
			break;
		++p;
	}

	fibril_mutex_unlock(&uart->buf_lock);

	*nread = p;
	return EOK;
}

static errno_t s3c24xx_uart_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	s3c24xx_uart_t *uart = (s3c24xx_uart_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		s3c24xx_uart_sendb(uart, dp[i]);

	*nwr = size;
	return EOK;
}


/** @}
 */
