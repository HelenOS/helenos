/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup kbd_port
 * @ingroup kbd
 * @{
 */
/** @file
 * @brief i8042 PS/2 port driver.
 */

#include <ddi.h>
#include <devman.h>
#include <device/hw_res.h>
#include <libarch/ddi.h>
#include <loc.h>
#include <async.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>

#include "i8042.h"

#define NAME       "i8042"

/* Interesting bits for status register */
#define i8042_OUTPUT_FULL	0x01
#define i8042_INPUT_FULL	0x02
#define i8042_AUX_DATA		0x20

/* Command constants */
#define i8042_CMD_WRITE_CMDB	0x60	/**< write command byte */
#define i8042_CMD_WRITE_AUX	0xd4	/**< write aux device */

/* Command byte fields */
#define i8042_KBD_IE		0x01
#define i8042_AUX_IE		0x02
#define i8042_KBD_DISABLE	0x10
#define i8042_AUX_DISABLE	0x20
#define i8042_KBD_TRANSLATE	0x40

static const irq_cmd_t i8042_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,	/* will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_BTEST,
		.value = i8042_OUTPUT_FULL,
		.srcarg = 1,
		.dstarg = 3
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 3
	},
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,	/* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

static void wait_ready(i8042_t *dev)
{
	assert(dev);
	while (pio_read_8(&dev->regs->status) & i8042_INPUT_FULL);
}

static void i8042_irq_handler(
    ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{

	const int status = IPC_GET_ARG1(*call);
	const int data = IPC_GET_ARG2(*call);
	const int devid = (status & i8042_AUX_DATA) ? DEVID_AUX : DEVID_PRI;
	ddf_msg(LVL_WARN, "Unhandled %s data: %x , status: %x.",
	    (devid == DEVID_AUX) ? "AUX" : "PRIMARY", data, status);
#if 0
	if (!dev || !dev->driver_data)
		return;

	if (device.port[devid].client_sess != NULL) {
		async_exch_t *exch =
		    async_exchange_begin(device.port[devid].client_sess);
		if (exch) {
			async_msg_1(exch, IPC_FIRST_USER_METHOD, data);
			async_exchange_end(exch);
		}
	} else {
		ddf_msg(LVL_WARN, "No client session.\n");
	}
#endif
}
/*----------------------------------------------------------------------------*/
int i8042_init(i8042_t *dev, void *regs, size_t reg_size, int irq_kbd,
    int irq_mouse, ddf_dev_t *ddf_dev)
{
	assert(ddf_dev);
	assert(dev);

	if (reg_size < sizeof(i8042_regs_t))
		return EINVAL;

	if (pio_enable(regs, sizeof(i8042_regs_t), (void**)&dev->regs) != 0)
		return -1;

	dev->kbd_fun = ddf_fun_create(ddf_dev, fun_exposed, "ps2a");
	if (!dev->kbd_fun)
		return ENOMEM;

	dev->mouse_fun = ddf_fun_create(ddf_dev, fun_exposed, "ps2b");
	if (!dev->mouse_fun) {
		ddf_fun_destroy(dev->kbd_fun);
		return ENOMEM;
	}

#define CHECK_RET_DESTROY(ret, msg...) \
if  (ret != EOK) { \
	ddf_msg(LVL_ERROR, msg); \
	if (dev->kbd_fun) { \
		dev->kbd_fun->driver_data = NULL; \
		ddf_fun_destroy(dev->kbd_fun); \
	} \
	if (dev->mouse_fun) { \
		dev->mouse_fun->driver_data = NULL; \
		ddf_fun_destroy(dev->mouse_fun); \
	} \
} else (void)0

	int ret = ddf_fun_bind(dev->kbd_fun);
	CHECK_RET_DESTROY(ret,
	    "Failed to bind keyboard function: %s.\n", str_error(ret));

	ret = ddf_fun_bind(dev->mouse_fun);
	CHECK_RET_DESTROY(ret,
	    "Failed to bind mouse function: %s.\n", str_error(ret));

	/* Disable kbd and aux */
	wait_ready(dev);
	pio_write_8(&dev->regs->status, i8042_CMD_WRITE_CMDB);
	wait_ready(dev);
	pio_write_8(&dev->regs->data, i8042_KBD_DISABLE | i8042_AUX_DISABLE);

	/* Flush all current IO */
	while (pio_read_8(&dev->regs->status) & i8042_OUTPUT_FULL)
		(void) pio_read_8(&dev->regs->data);

#define CHECK_RET_UNBIND_DESTROY(ret, msg...) \
if  (ret != EOK) { \
	ddf_msg(LVL_ERROR, msg); \
	if (dev->kbd_fun) { \
		ddf_fun_unbind(dev->kbd_fun); \
		dev->kbd_fun->driver_data = NULL; \
		ddf_fun_destroy(dev->kbd_fun); \
	} \
	if (dev->mouse_fun) { \
		ddf_fun_unbind(dev->mouse_fun); \
		dev->mouse_fun->driver_data = NULL; \
		ddf_fun_destroy(dev->mouse_fun); \
	} \
} else (void)0

	const size_t cmd_count = sizeof(i8042_cmds) / sizeof(irq_cmd_t);
	irq_cmd_t cmds[cmd_count];
	memcpy(cmds, i8042_cmds, sizeof(i8042_cmds));
	cmds[0].addr = (void *) &dev->regs->status;
	cmds[3].addr = (void *) &dev->regs->data;

	irq_code_t irq_code = { .cmdcount = cmd_count, .cmds = cmds };
	ret = register_interrupt_handler(ddf_dev, irq_kbd, i8042_irq_handler,
	    &irq_code);
	CHECK_RET_UNBIND_DESTROY(ret,
	    "Failed set handler for kbd: %s.\n", str_error(ret));

	ret = register_interrupt_handler(ddf_dev, irq_mouse, i8042_irq_handler,
	    &irq_code);
	CHECK_RET_UNBIND_DESTROY(ret,
	    "Failed set handler for mouse: %s.\n", str_error(ret));

#if 0
	ret = ddf_fun_add_to_category(dev->kbd_fun, "keyboard");
	if (ret != EOK)
		ddf_msg(LVL_WARN, "Failed to register kbd fun to category.\n");
	ret = ddf_fun_add_to_category(dev->mouse_fun, "mouse");
	if (ret != EOK)
		ddf_msg(LVL_WARN, "Failed to register mouse fun to category.\n");
#endif
	/* Enable interrupts */
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, ddf_dev->handle,
	    IPC_FLAG_BLOCKING);
	ret = parent_sess ? EOK : ENOMEM;
	CHECK_RET_UNBIND_DESTROY(ret, "Failed to create parent connection.\n");
	const bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);
	ret = enabled ? EOK : EIO;
	CHECK_RET_UNBIND_DESTROY(ret, "Failed to enable interrupts: %s.\n");


	wait_ready(dev);
	pio_write_8(&dev->regs->status, i8042_CMD_WRITE_CMDB);
	wait_ready(dev);
	pio_write_8(&dev->regs->data, i8042_KBD_IE | i8042_KBD_TRANSLATE |
	    i8042_AUX_IE);

	return EOK;
}

/** Character device connection handler */
#if 0
static void i8042_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ipc_callid_t callid;
	ipc_call_t call;
	sysarg_t method;
	service_id_t dsid;
	int retval;
	int dev_id, i;

	printf(NAME ": connection handler\n");

	/* Get the device handle. */
	dsid = IPC_GET_ARG1(*icall);

	/* Determine which disk device is the client connecting to. */
	dev_id = -1;
	for (i = 0; i < MAX_DEVS; i++) {
		if (device.port[i].service_id == dsid)
			dev_id = i;
	}

	if (dev_id < 0) {
		async_answer_0(iid, EINVAL);
		return;
	}

	/* Answer the IPC_M_CONNECT_ME_TO call. */
	async_answer_0(iid, EOK);

	printf(NAME ": accepted connection\n");

	while (1) {
		callid = async_get_call(&call);
		method = IPC_GET_IMETHOD(call);
		
		if (!method) {
			/* The other side has hung up. */
			async_answer_0(callid, EOK);
			return;
		}
		
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, &call);
		if (sess != NULL) {
			if (device.port[dev_id].client_sess == NULL) {
				device.port[dev_id].client_sess = sess;
				retval = EOK;
			} else
				retval = ELIMIT;
		} else {
			switch (method) {
			case IPC_FIRST_USER_METHOD:
				printf(NAME ": write %" PRIun " to devid %d\n",
				    IPC_GET_ARG1(call), dev_id);
				i8042_port_write(&device, dev_id, IPC_GET_ARG1(call));
				retval = 0;
				break;
			default:
				retval = EINVAL;
				break;
			}
		}
		
		async_answer_0(callid, retval);
	}
}
void i8042_port_write(i8042_t *dev, int devid, uint8_t data)
{

	assert(dev);
	if (devid == DEVID_AUX) {
		wait_ready(dev);
		pio_write_8(&dev->regs->status, i8042_CMD_WRITE_AUX);
	}
	wait_ready(dev);
	pio_write_8(&dev->regs->data, data);
}
#endif

/**
 * @}
 */
