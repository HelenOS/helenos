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

#include <ipc/irc.h>
#include <loc.h>
#include <sysinfo.h>
#include <as.h>
#include <ddf/log.h>
#include <ddi.h>
#include <align.h>
#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <async.h>
#include <stdio.h>

#include "i8259.h"

#define NAME  "i8259"

#define IO_RANGE0_SIZE   2
#define IO_RANGE1_SIZE   2

#define PIC_PIC0PORT1  0
#define PIC_PIC0PORT2  1

#define PIC_PIC1PORT1  0
#define PIC_PIC1PORT2  1

#define PIC_MAX_IRQ  15

static errno_t pic_enable_irq(i8259_t *i8259, sysarg_t irq)
{
	if (irq > PIC_MAX_IRQ)
		return ENOENT;
	
	uint16_t irqmask = 1 << irq;
	uint8_t val;
	
	if (irqmask & 0xff) {
		val = pio_read_8(i8259->regs0 + PIC_PIC0PORT2);
		pio_write_8(i8259->regs0 + PIC_PIC0PORT2,
		    (uint8_t) (val & (~(irqmask & 0xff))));
	}
	
	if (irqmask >> 8) {
		val = pio_read_8(i8259->regs1 + PIC_PIC1PORT2);
		pio_write_8(i8259->regs1 + PIC_PIC1PORT2,
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
	i8259_t *i8259 = NULL /* XXX */;
	
	/*
	 * Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	async_answer_0(iid, EOK);
	
	i8259 = (i8259_t *)ddf_dev_data_get(ddf_fun_get_dev((ddf_fun_t *)arg));
	
	while (true) {
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case IRC_ENABLE_INTERRUPT:
			async_answer_0(callid, pic_enable_irq(i8259,
			    IPC_GET_ARG1(call)));
			break;
		case IRC_DISABLE_INTERRUPT:
			/* XXX TODO */
			async_answer_0(callid, EOK);
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

/** Add i8259 device. */
errno_t i8259_add(i8259_t *i8259, i8259_res_t *res)
{
	sysarg_t have_i8259;
	ioport8_t *regs0;
	ioport8_t *regs1;
	ddf_fun_t *fun_a = NULL;
	errno_t rc;
	
	if ((sysinfo_get_value("i8259", &have_i8259) != EOK) || (!have_i8259)) {
		printf("%s: No i8259 found\n", NAME);
		return ENOTSUP;
	}
	
	if ((pio_enable((void *) res->base0, IO_RANGE0_SIZE,
	    (void **) &regs0) != EOK) ||
	    (pio_enable((void *) res->base1, IO_RANGE1_SIZE,
	    (void **) &regs1) != EOK)) {
		printf("%s: i8259 not accessible\n", NAME);
		return EIO;
	}
	
	i8259->regs0 = regs0;
	i8259->regs1 = regs1;
	
	fun_a = ddf_fun_create(i8259->dev, fun_exposed, "a");
	if (fun_a == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}
	
	ddf_fun_set_conn_handler(fun_a, i8259_connection);
	
	rc = ddf_fun_bind(fun_a);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'a': %s", str_error(rc));
		goto error;
	}
	
	rc = ddf_fun_add_to_category(fun_a, "irc");
	if (rc != EOK)
		goto error;
	
	return EOK;
error:
	if (fun_a != NULL)
		ddf_fun_destroy(fun_a);
	return rc;
}

/** Remove i8259 device */
errno_t i8259_remove(i8259_t *i8259)
{
	return ENOTSUP;
}

/** i8259 device gone */
errno_t i8259_gone(i8259_t *i8259)
{
	return ENOTSUP;
}

/**
 * @}
 */
