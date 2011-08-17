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

/** @addtogroup mouse_port
 * @ingroup mouse
 * @{
 */
/** @file
 * @brief Chardev mouse port driver.
 */

#include <ipc/char.h>
#include <stdio.h>
#include <async.h>
#include <errno.h>
#include <loc.h>
#include <input.h>
#include <mouse_port.h>
#include <mouse.h>

static mouse_dev_t *mouse_dev;
static async_sess_t *dev_sess;

/** List of devices to try connecting to. */
static const char *in_devs[] = {
	"char/ps2b",
};

static const unsigned int num_devs = sizeof(in_devs) / sizeof(in_devs[0]);

static void mouse_port_events(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Ignore parameters, the connection is already opened */
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			/* TODO: Handle hangup */
			return;
		}
		
		int retval;
		
		switch (IPC_GET_IMETHOD(call)) {
		case CHAR_NOTIF_BYTE:
			mouse_push_data(mouse_dev, IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}
		
		async_answer_0(callid, retval);
	}
}

static int chardev_port_init(mouse_dev_t *mdev)
{
	service_id_t service_id;
	unsigned int i;
	int rc;
	
	mouse_dev = mdev;
	
	for (i = 0; i < num_devs; i++) {
		rc = loc_service_get_id(in_devs[i], &service_id, 0);
		if (rc == EOK)
			break;
	}
	
	if (i >= num_devs) {
		printf("%s: Could not find any suitable input device\n", NAME);
		return -1;
	}
	
	dev_sess = loc_service_connect(EXCHANGE_ATOMIC, service_id,
	    IPC_FLAG_BLOCKING);
	if (dev_sess == NULL) {
		printf("%s: Failed connecting to device\n", NAME);
		return ENOENT;
	}
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	if (exch == NULL) {
		printf("%s: Failed starting exchange with device\n", NAME);
		async_hangup(dev_sess);
		return ENOMEM;
	}
	
	/* NB: The callback connection is slotted for removal */
	rc = async_connect_to_me(exch, 0, 0, 0, mouse_port_events, NULL);
	async_exchange_end(exch);
	
	if (rc != 0) {
		printf("%s: Failed to create callback from device\n", NAME);
		async_hangup(dev_sess);
		return -1;
	}
	
	return 0;
}

static void chardev_port_yield(void)
{
}

static void chardev_port_reclaim(void)
{
}

static void chardev_port_write(uint8_t data)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	if (exch == NULL) {
		printf("%s: Failed starting exchange with device\n", NAME);
		return;
	}

	async_msg_1(exch, CHAR_WRITE_BYTE, data);
	async_exchange_end(exch);
}

mouse_port_ops_t chardev_mouse_port = {
	.init = chardev_port_init,
	.yield = chardev_port_yield,
	.reclaim = chardev_port_reclaim,
	.write = chardev_port_write
};

/**
 * @}
 */