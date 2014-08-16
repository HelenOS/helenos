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

/** @addtogroup driver_serial
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 *
 * This UART is present on the Samsung S3C24xx CPU (on the gta02 platform).
 */

#include <ddi.h>
#include <loc.h>
#include <ipc/char.h>
#include <async.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysinfo.h>
#include <errno.h>
#include <inttypes.h>
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

static void s3c24xx_uart_connection(ipc_callid_t, ipc_call_t *, void *);
static void s3c24xx_uart_irq_handler(ipc_callid_t, ipc_call_t *, void *);
static int s3c24xx_uart_init(s3c24xx_uart_t *);
static void s3c24xx_uart_sendb(s3c24xx_uart_t *, uint8_t);

int main(int argc, char *argv[])
{
	printf("%s: S3C24xx on-chip UART driver\n", NAME);
	
	async_set_client_connection(s3c24xx_uart_connection);
	int rc = loc_server_register(NAME);
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
static void s3c24xx_uart_connection(ipc_callid_t iid, ipc_call_t *icall,
    void *arg)
{
	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);
		
		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			if (uart->client_sess == NULL) {
				uart->client_sess = sess;
				async_answer_0(callid, EOK);
			} else
				async_answer_0(callid, ELIMIT);
		} else {
			switch (method) {
			case CHAR_WRITE_BYTE:
				printf(NAME ": write %" PRIun " to device\n",
				    IPC_GET_ARG1(call));
				s3c24xx_uart_sendb(uart, (uint8_t) IPC_GET_ARG1(call));
				async_answer_0(callid, EOK);
				break;
			default:
				async_answer_0(callid, EINVAL);
			}
		}
	}
}

static void s3c24xx_uart_irq_handler(ipc_callid_t iid, ipc_call_t *call,
    void *arg)
{
	(void) iid;
	(void) call;
	(void) arg;

	while ((pio_read_32(&uart->io->ufstat) & S3C24XX_UFSTAT_RX_COUNT) != 0) {
		uint32_t data = pio_read_32(&uart->io->urxh) & 0xff;
		uint32_t status = pio_read_32(&uart->io->uerstat);

		if (uart->client_sess != NULL) {
			async_exch_t *exch = async_exchange_begin(uart->client_sess);
			async_msg_1(exch, CHAR_NOTIF_BYTE, data);
			async_exchange_end(exch);
		}

		if (status != 0)
			printf(NAME ": Error status 0x%x\n", status);
	}
}

/** Initialize S3C24xx on-chip UART. */
static int s3c24xx_uart_init(s3c24xx_uart_t *uart)
{
	void *vaddr;
	sysarg_t inr;

	if (sysinfo_get_value("s3c24xx_uart.address.physical",
	    &uart->paddr) != EOK)
		return -1;

	if (pio_enable((void *) uart->paddr, sizeof(s3c24xx_uart_io_t),
	    &vaddr) != 0)
		return -1;

	if (sysinfo_get_value("s3c24xx_uart.inr", &inr) != EOK)
		return -1;

	uart->io = vaddr;
	uart->client_sess = NULL;

	printf(NAME ": device at physical address %p, inr %" PRIun ".\n",
	    (void *) uart->paddr, inr);

	async_irq_subscribe(inr, device_assign_devno(), s3c24xx_uart_irq_handler,
	    NULL, &uart_irq_code);

	/* Enable FIFO, Tx trigger level: empty, Rx trigger level: 1 byte. */
	pio_write_32(&uart->io->ufcon, UFCON_FIFO_ENABLE |
	    UFCON_TX_FIFO_TLEVEL_EMPTY | UFCON_RX_FIFO_TLEVEL_1B);

	/* Set RX interrupt to pulse mode */
	pio_write_32(&uart->io->ucon,
	    pio_read_32(&uart->io->ucon) & ~UCON_RX_INT_LEVEL);

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

/** @}
 */
