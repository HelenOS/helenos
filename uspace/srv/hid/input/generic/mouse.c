/*
 * Copyright (c) 2011 Jiri Svoboda
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
 * @addtogroup inputgen generic
 * @brief Mouse device handling.
 * @ingroup input
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <async.h>
#include <errno.h>
#include <fcntl.h>
#include <input.h>
#include <ipc/mouse.h>
#include <mouse.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs_sess.h>

static void mouse_callback_conn(ipc_callid_t, ipc_call_t *, void *);

static mouse_dev_t *mouse_dev_new(void)
{
	mouse_dev_t *mdev;

	mdev = calloc(1, sizeof(mouse_dev_t));
	if (mdev == NULL) {
		printf(NAME ": Error allocating mouse device. "
		    "Out of memory.\n");
		return NULL;
	}

	link_initialize(&mdev->mouse_devs);
	return mdev;
}

static int mouse_dev_connect(mouse_dev_t *mdev, const char *dev_path)
{
	async_sess_t *sess;
	async_exch_t *exch;
	int fd;
	int rc;

	fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	sess = fd_session(EXCHANGE_SERIALIZE, fd);
	if (sess == NULL) {
		printf(NAME ": Failed starting session with '%s'\n", dev_path);
		close(fd);
		return -1;
	}

	exch = async_exchange_begin(sess);
	if (exch == NULL) {
		printf(NAME ": Failed starting exchange with '%s'.\n", dev_path);
		return -1;
	}

	rc = async_connect_to_me(exch, 0, 0, 0, mouse_callback_conn, mdev);
	if (rc != EOK) {
		printf(NAME ": Failed creating callback connection from '%s'.\n",
		    dev_path);
		async_exchange_end(exch);
		return -1;
	}

	async_exchange_end(exch);

	mdev->dev_path = dev_path;
	return 0;
}

/** Add new mouse device.
 *
 * @param dev_path	Filesystem path to the device (/dev/class/...)
 */
int mouse_add_dev(const char *dev_path)
{
	mouse_dev_t *mdev;
	int rc;

	mdev = mouse_dev_new();
	if (mdev == NULL)
		return -1;

	rc = mouse_dev_connect(mdev, dev_path);
	if (rc != EOK) {
		free(mdev);
		return -1;
	}

	list_append(&mdev->mouse_devs, &mouse_devs);
	return EOK;
}

/** Mouse device callback connection handler. */
static void mouse_callback_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	int retval;

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid;

		callid = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call)) {
			/* XXX Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case MEVENT_BUTTON:
			input_event_button(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			retval = 0;
			break;
		case MEVENT_MOVE:
			input_event_move(IPC_GET_ARG1(call),
			    IPC_GET_ARG2(call));
			retval = 0;
			break;
		default:
			retval = ENOTSUP;
			break;
		}

		async_answer_0(callid, retval);
	}
}

/**
 * @}
 */
