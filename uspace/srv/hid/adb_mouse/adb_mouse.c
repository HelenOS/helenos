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
 * @addtogroup mouse
 * @brief ADB Apple classic mouse driver.
 *
 * This driver handles a mouse connected to Apple Desktop Bus speaking
 * the Apple classic protocol. It connects to an ADB driver.
 *
 * @{
 */
/** @file
 */

#include <ipc/ipc.h>
#include <ipc/mouse.h>
#include <stdio.h>
#include <stdlib.h>
#include <async.h>
#include <errno.h>
#include <devmap.h>

#include "adb_mouse.h"
#include "adb_dev.h"

static void client_connection(ipc_callid_t iid, ipc_call_t *icall);
static void mouse_ev_btn(int button, int press);
static void mouse_ev_move(int dx, int dy);

static int client_phone = -1;
static bool b1_pressed, b2_pressed;

int main(int argc, char **argv)
{
	printf(NAME ": Chardev mouse driver\n");

	/* Initialize device. */
	if (adb_dev_init() != 0)
		return -1;

	b1_pressed = false;
	b2_pressed = false; 

	/* Register driver */
	int rc = devmap_driver_register(NAME, client_connection);
	if (rc < 0) {
		printf(NAME ": Unable to register driver (%d)\n", rc);
		return -1;
	}

	char dev_path[DEVMAP_NAME_MAXLEN + 1];
	snprintf(dev_path, DEVMAP_NAME_MAXLEN, "%s/mouse", NAMESPACE);

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

void mouse_handle_data(uint16_t data)
{
	bool b1, b2;
	uint16_t udx, udy;
	int dx, dy;

	/* Extract fields. */
	b1 = ((data >> 15) & 1) == 0;
	udy = (data >> 8) & 0x7f;
	b2 = ((data >> 7) & 1) == 0;
	udx = data & 0x7f;

	/* Decode 7-bit two's complement signed values. */
	dx = (udx & 0x40) ? (udx - 0x80) : udx;
	dy = (udy & 0x40) ? (udy - 0x80) : udy;

	if (b1 != b1_pressed) {
		mouse_ev_btn(1, b1);
		b1_pressed = b1;
	}

	if (b2 != b2_pressed) {
		mouse_ev_btn(2, b2);
		b1_pressed = b1;
	}

	if (dx != 0 || dy != 0)
		mouse_ev_move(dx, dy);
}

static void mouse_ev_btn(int button, int press)
{
	if (client_phone != -1) {
		async_msg_2(client_phone, MEVENT_BUTTON, button, press);
	}
}

static void mouse_ev_move(int dx, int dy)
{
	if (client_phone != -1)
		async_msg_2(client_phone, MEVENT_MOVE, dx, dy);
}

/**
 * @}
 */
