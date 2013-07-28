/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup i8259
 * @{
 */

/**
 * @file i8259.c
 * @brief i8259 driver.
 */

#include <ipc/services.h>
#include <ipc/irc.h>
#include <ns.h>
#include <sysinfo.h>
#include <as.h>
#include <ddi.h>
#include <align.h>
#include <stdbool.h>
#include <errno.h>
#include <async.h>
#include <align.h>
#include <async.h>
#include <stdio.h>
#include <ipc/loc.h>

#define NAME  "i8259"

#define IO_RANGE0_START  ((ioport8_t *) 0x0020U)
#define IO_RANGE0_SIZE   2

#define IO_RANGE1_START  ((ioport8_t *) 0x00a0U)
#define IO_RANGE1_SIZE   2

static ioport8_t *io_range0;
static ioport8_t *io_range1;

#define PIC_PIC0PORT1  0
#define PIC_PIC0PORT2  1

#define PIC_PIC1PORT1  0
#define PIC_PIC1PORT2  1

#define PIC_MAX_IRQ  15

static int pic_enable_irq(sysarg_t irq)
{
	if (irq > PIC_MAX_IRQ)
		return ENOENT;
	
	uint16_t irqmask = 1 << irq;
	uint8_t val;
	
	if (irqmask & 0xff) {
		val = pio_read_8(io_range0 + PIC_PIC0PORT2);
		pio_write_8(io_range0 + PIC_PIC0PORT2,
		    (uint8_t) (val & (~(irqmask & 0xff))));
	}
	
	if (irqmask >> 8) {
		val = pio_read_8(io_range1 + PIC_PIC1PORT2);
		pio_write_8(io_range1 + PIC_PIC1PORT2,
		    (uint8_t) (val & (~(irqmask >> 8))));
	}
	
	return EOK;
}

/** Handle one connection to i8259.
 *
 * @param iid   Hash of the request that opened the connection.
 * @param icall Call data of the request that opened the connection.
 * @param arg	Local argument.
 */
static void i8259_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	while (true) {
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(callid, pic_enable_irq(IPC_GET_ARG1(call)));
			break;
		case IRC_CLEAR_INTERRUPT:
			/* Noop */
			async_answer_0(callid, EOK);
			break;
		default:
			async_answer_0(callid, EINVAL);
			break;
		}
	}
}

/** Initialize the i8259 driver.
 *
 */
static bool i8259_init(void)
{
	sysarg_t i8259;
	
	if ((sysinfo_get_value("i8259", &i8259) != EOK) || (!i8259)) {
		printf("%s: No i8259 found\n", NAME);
		return false;
	}
	
	if ((pio_enable((void *) IO_RANGE0_START, IO_RANGE0_SIZE,
	    (void **) &io_range0) != EOK) ||
	    (pio_enable((void *) IO_RANGE1_START, IO_RANGE1_SIZE,
	    (void **) &io_range1) != EOK)) {
		printf("%s: i8259 not accessible\n", NAME);
		return false;
	}
	
	async_set_client_connection(i8259_connection);
	service_register(SERVICE_IRC);
	
	return true;
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS i8259 driver\n", NAME);
	
	if (!i8259_init())
		return -1;
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached */
	return 0;
}

/**
 * @}
 */
