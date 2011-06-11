/*
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
 * @brief Chardev keyboard port driver.
 */

#include <ipc/char.h>
#include <async.h>
#include <async_obsolete.h>
#include <kbd_port.h>
#include <kbd.h>
#include <devmap.h>
#include <devmap_obsolete.h>
#include <errno.h>
#include <stdio.h>

#define NAME  "kbd/chardev"

static void kbd_port_events(ipc_callid_t iid, ipc_call_t *icall);

static int chardev_port_init(void);
static void chardev_port_yield(void);
static void chardev_port_reclaim(void);
static void chardev_port_write(uint8_t data);

kbd_port_ops_t chardev_port = {
	.init = chardev_port_init,
	.yield = chardev_port_yield,
	.reclaim = chardev_port_reclaim,
	.write = chardev_port_write
};

static int dev_phone;

/** List of devices to try connecting to. */
static const char *in_devs[] = {
	"char/ps2a",
	"char/s3c24ser"
};

static const unsigned int num_devs = sizeof(in_devs) / sizeof(in_devs[0]);

static int chardev_port_init(void)
{
	devmap_handle_t handle;
	unsigned int i;
	int rc;
	
	for (i = 0; i < num_devs; i++) {
		rc = devmap_device_get_handle(in_devs[i], &handle, 0);
		if (rc == EOK)
			break;
	}
	
	if (i >= num_devs) {
		printf("%s: Could not find any suitable input device\n", NAME);
		return -1;
	}
	
	dev_phone = devmap_obsolete_device_connect(handle, IPC_FLAG_BLOCKING);
	if (dev_phone < 0) {
		printf("%s: Failed connecting to device\n", NAME);
		return ENOENT;
	}
	
	/* NB: The callback connection is slotted for removal */
	if (async_obsolete_connect_to_me(dev_phone, 0, 0, 0, kbd_port_events) != 0) {
		printf(NAME ": Failed to create callback from device\n");
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
	async_obsolete_msg_1(dev_phone, CHAR_WRITE_BYTE, data);
}

static void kbd_port_events(ipc_callid_t iid, ipc_call_t *icall)
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
			kbd_push_scancode(IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}
		async_answer_0(callid, retval);
	}
}


/**
 * @}
 */ 
