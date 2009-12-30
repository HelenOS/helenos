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

/**
 * @addtogroup mouse
 * @brief Chardev mouse driver.
 *
 * This is a common driver for mice attached to simple character devices
 * (PS/2 mice, serial mice).
 *
 * @{
 */
/** @file
 */

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/mouse.h>
#include <sysinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ipc/ns.h>
#include <async.h>
#include <errno.h>
#include <adt/fifo.h>
#include <io/console.h>
#include <io/keycode.h>
#include <devmap.h>

#include <c_mouse.h>
#include <mouse_port.h>
#include <mouse_proto.h>

#define NAME       "mouse"
#define NAMESPACE  "hid_in"

int client_phone = -1;

void mouse_handle_byte(int byte)
{
/*	printf("mouse byte: 0x%x\n", byte);*/
	mouse_proto_parse_byte(byte);
}

void mouse_ev_btn(int button, int press)
{
/*	printf("ev_btn: button %d, press %d\n", button, press);*/
	if (client_phone != -1) {
		async_msg_2(client_phone, MEVENT_BUTTON, button, press);
	}
}

void mouse_ev_move(int dx, int dy)
{
/*	printf("ev_move: dx %d, dy %d\n", dx, dy);*/
	if (client_phone != -1)
		async_msg_2(client_phone, MEVENT_MOVE, dx, dy);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	ipc_callid_t callid;
	ipc_call_t call;
	int retval;

	ipc_answer_0(iid, EOK);

	while (1) {
		callid = async_get_call(&call);
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			if (client_phone != -1) {
				ipc_hangup(client_phone);
				client_phone = -1;
			}

			ipc_answer_0(callid, EOK);
			return;
		case IPC_M_CONNECT_TO_ME:
			if (client_phone != -1) {
				retval = ELIMIT;
				break;
			}
			client_phone = IPC_GET_ARG5(call);
			retval = 0;
			break;
		default:
			retval = EINVAL;
		}
		ipc_answer_0(callid, retval);
	}
}


int main(int argc, char **argv)
{
	printf(NAME ": Chardev mouse driver\n");

	/* Initialize port. */
	if (mouse_port_init() != 0)
		return -1;

	/* Initialize protocol driver. */
	if (mouse_proto_init() != 0)
		return -1;

	/* Register driver */
	int rc = devmap_driver_register(NAME, client_connection);
	if (rc < 0) {
		printf(NAME ": Unable to register driver (%d)\n", rc);
		return -1;
	}

	char dev_path[DEVMAP_NAME_MAXLEN + 1];
	snprintf(dev_path, DEVMAP_NAME_MAXLEN, "%s/%s", NAMESPACE, NAME);

	dev_handle_t dev_handle;
	if (devmap_device_register(dev_path, &dev_handle) != EOK) {
		printf(NAME ": Unable to register device %s\n", dev_path);
		return -1;
	}

	printf(NAME ": Accepting connections\n");
	task_retval(0);
	async_manager();

	/* Not reached. */
	return 0;
}

/**
 * @}
 */
