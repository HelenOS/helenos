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

/** @addtogroup mouse
 * @{
 */ 
/** @file
 * @brief
 */

#include <ipc/ipc.h>
#include <ipc/adb.h>
#include <async.h>
#include <vfs/vfs.h>
#include <fcntl.h>
#include <errno.h>

#include "adb_mouse.h"
#include "adb_dev.h"

static void adb_dev_events(ipc_callid_t iid, ipc_call_t *icall);

static int dev_phone;

int adb_dev_init(void)
{
	char *input = "/dev/adb/mouse";
	int input_fd;

	printf(NAME ": open %s\n", input);

	input_fd = open(input, O_RDONLY);
	if (input_fd < 0) {
		printf(NAME ": Failed opening %s (%d)\n", input, input_fd);
		return false;
	}

	dev_phone = fd_phone(input_fd);
	if (dev_phone < 0) {
		printf(NAME ": Failed to connect to device\n");
		return false;
	}

	/* NB: The callback connection is slotted for removal */
	ipcarg_t phonehash;
	if (ipc_connect_to_me(dev_phone, 0, 0, 0, &phonehash) != 0) {
		printf(NAME ": Failed to create callback from device\n");
		return false;
	}

	async_new_connection(phonehash, 0, NULL, adb_dev_events);

	return 0;
}

static void adb_dev_events(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Ignore parameters, the connection is already opened */
	while (true) {

		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		int retval;

		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			/* TODO: Handle hangup */
			return;
		case IPC_FIRST_USER_METHOD:
			mouse_handle_data(IPC_GET_ARG1(call));
			break;
		default:
			retval = ENOENT;
		}
		ipc_answer_0(callid, retval);
	}
}

/**
 * @}
 */
