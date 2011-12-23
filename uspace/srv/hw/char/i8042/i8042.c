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
#include <libarch/ddi.h>
#include <loc.h>
#include <async.h>
#include <unistd.h>
#include <sysinfo.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include "i8042.h"

#define NAME       "i8042"
#define NAMESPACE  "char"

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


enum {
	DEVID_PRI = 0, /**< primary device */
        DEVID_AUX = 1, /**< AUX device */
	MAX_DEVS  = 2
};

static irq_cmd_t i8042_cmds[] = {
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

static irq_code_t i8042_kbd = {
	sizeof(i8042_cmds) / sizeof(irq_cmd_t),
	i8042_cmds
};

static uintptr_t i8042_physical;
static uintptr_t i8042_kernel;
static i8042_t * i8042;

static i8042_port_t i8042_port[MAX_DEVS];

static void wait_ready(void)
{
	while (pio_read_8(&i8042->status) & i8042_INPUT_FULL);
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call);
static void i8042_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg);
static int i8042_init(void);
static void i8042_port_write(int devid, uint8_t data);


int main(int argc, char *argv[])
{
	char name[16];
	int i, rc;
	char dchar[MAX_DEVS] = { 'a', 'b' };

	printf(NAME ": i8042 PS/2 port driver\n");

	rc = loc_server_register(NAME, i8042_connection);
	if (rc < 0) {
		printf(NAME ": Unable to register server.\n");
		return rc;
	}

	if (i8042_init() != EOK)
		return -1;

	for (i = 0; i < MAX_DEVS; i++) {
		i8042_port[i].client_sess = NULL;

		snprintf(name, 16, "%s/ps2%c", NAMESPACE, dchar[i]);
		rc = loc_service_register(name, &i8042_port[i].service_id);
		if (rc != EOK) {
			printf(NAME ": Unable to register device %s.\n", name);
			return rc;
		}
		printf(NAME ": Registered device %s\n", name);
	}

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}

static int i8042_init(void)
{
	if (sysinfo_get_value("i8042.address.physical", &i8042_physical) != EOK)
		return -1;
	
	if (sysinfo_get_value("i8042.address.kernel", &i8042_kernel) != EOK)
		return -1;
	
	void *vaddr;
	if (pio_enable((void *) i8042_physical, sizeof(i8042_t), &vaddr) != 0)
		return -1;
	
	i8042 = vaddr;
	
	sysarg_t inr_a;
	sysarg_t inr_b;
	
	if (sysinfo_get_value("i8042.inr_a", &inr_a) != EOK)
		return -1;
	
	if (sysinfo_get_value("i8042.inr_b", &inr_b) != EOK)
		return -1;
	
	async_set_interrupt_received(i8042_irq_handler);
	
	/* Disable kbd and aux */
	wait_ready();
	pio_write_8(&i8042->status, i8042_CMD_WRITE_CMDB);
	wait_ready();
	pio_write_8(&i8042->data, i8042_KBD_DISABLE | i8042_AUX_DISABLE);

	/* Flush all current IO */
	while (pio_read_8(&i8042->status) & i8042_OUTPUT_FULL)
		(void) pio_read_8(&i8042->data);

	i8042_kbd.cmds[0].addr = (void *) &((i8042_t *) i8042_kernel)->status;
	i8042_kbd.cmds[3].addr = (void *) &((i8042_t *) i8042_kernel)->data;
	irq_register(inr_a, device_assign_devno(), 0, &i8042_kbd);
	irq_register(inr_b, device_assign_devno(), 0, &i8042_kbd);
	printf("%s: registered for interrupts %" PRIun " and %" PRIun "\n",
	    NAME, inr_a, inr_b);

	wait_ready();
	pio_write_8(&i8042->status, i8042_CMD_WRITE_CMDB);
	wait_ready();
	pio_write_8(&i8042->data, i8042_KBD_IE | i8042_KBD_TRANSLATE |
	    i8042_AUX_IE);

	return 0;
}

/** Character device connection handler */
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
		if (i8042_port[i].service_id == dsid)
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
			if (i8042_port[dev_id].client_sess == NULL) {
				i8042_port[dev_id].client_sess = sess;
				retval = EOK;
			} else
				retval = ELIMIT;
		} else {
			switch (method) {
			case IPC_FIRST_USER_METHOD:
				printf(NAME ": write %" PRIun " to devid %d\n",
				    IPC_GET_ARG1(call), dev_id);
				i8042_port_write(dev_id, IPC_GET_ARG1(call));
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

void i8042_port_write(int devid, uint8_t data)
{
	if (devid == DEVID_AUX) {
		wait_ready();
		pio_write_8(&i8042->status, i8042_CMD_WRITE_AUX);
	}
	wait_ready();
	pio_write_8(&i8042->data, data);
}

static void i8042_irq_handler(ipc_callid_t iid, ipc_call_t *call)
{
	int status, data;
	int devid;

	status = IPC_GET_ARG1(*call);
	data = IPC_GET_ARG2(*call);

	if ((status & i8042_AUX_DATA)) {
		devid = DEVID_AUX;
	} else {
		devid = DEVID_PRI;
	}

	if (i8042_port[devid].client_sess != NULL) {
		async_exch_t *exch =
		    async_exchange_begin(i8042_port[devid].client_sess);
		async_msg_1(exch, IPC_FIRST_USER_METHOD, data);
		async_exchange_end(exch);
	}
}

/**
 * @}
 */
