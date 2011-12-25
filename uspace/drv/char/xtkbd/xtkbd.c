/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvkbd
 * @{
 */
/** @file
 * @brief XT keyboard driver;
 */

#include <errno.h>
#include <devman.h>
#include <device/char_dev.h>
#include <ddf/log.h>
#include <ipc/kbdev.h>
#include <abi/ipc/methods.h>

#include "xtkbd.h"

static int polling(void *);
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall);

static ddf_dev_ops_t kbd_ops = {
	.default_handler = default_connection_handler
};

int xt_kbd_init(xt_kbd_t *kbd, ddf_dev_t *dev)
{
	assert(kbd);
	assert(dev);
	kbd->input_sess = NULL;
	kbd->parent_sess = devman_parent_device_connect(EXCHANGE_SERIALIZE,
	    dev->handle, IPC_FLAG_BLOCKING);
	if (!kbd->parent_sess)
		return ENOMEM;

	kbd->kbd_fun = ddf_fun_create(dev, fun_exposed, "kbd");
	if (!kbd->kbd_fun) {
		async_hangup(kbd->parent_sess);
		return ENOMEM;
	}
	kbd->kbd_fun->ops = &kbd_ops;
	kbd->kbd_fun->driver_data = kbd;

	int ret = ddf_fun_bind(kbd->kbd_fun);
	if (ret != EOK) {
		async_hangup(kbd->parent_sess);
		kbd->kbd_fun->driver_data = NULL;
		ddf_fun_destroy(kbd->kbd_fun);
		return ENOMEM;
	}

	ret = ddf_fun_add_to_category(kbd->kbd_fun, "keyboard");
	if (ret != EOK) {
		async_hangup(kbd->parent_sess);
		ddf_fun_unbind(kbd->kbd_fun);
		kbd->kbd_fun->driver_data = NULL;
		ddf_fun_destroy(kbd->kbd_fun);
		return ENOMEM;
	}

	kbd->polling_fibril = fibril_create(polling, kbd);
	if (!kbd->polling_fibril) {
		async_hangup(kbd->parent_sess);
		ddf_fun_unbind(kbd->kbd_fun);
		kbd->kbd_fun->driver_data = NULL;
		ddf_fun_destroy(kbd->kbd_fun);
		return ENOMEM;
	}
	fibril_add_ready(kbd->polling_fibril);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int polling(void *arg)
{
	assert(arg);
	xt_kbd_t *kbd = arg;

	assert(kbd->parent_sess);
	while (1) {
		uint8_t code = 0;
		ssize_t size = char_dev_read(kbd->parent_sess, &code, 1);
		ddf_msg(LVL_DEBUG, "Got scancode: %hhx, size: %zd", code, size);
	}
}
/*----------------------------------------------------------------------------*/
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	if (fun == NULL || fun->driver_data == NULL) {
		ddf_msg(LVL_ERROR, "%s: Missing parameter.\n", __FUNCTION__);
		async_answer_0(icallid, EINVAL);
		return;
	}

	const sysarg_t method = IPC_GET_IMETHOD(*icall);
	xt_kbd_t *kbd = fun->driver_data;

	switch (method) {
	case KBDEV_SET_IND:
		/* XT keyboards do not support setting mods */
		async_answer_0(icallid, ENOTSUP);
		break;
	/* This might be ugly but async_callback_receive_start makes no
	 * difference for incorrect call and malloc failure. */
	case IPC_M_CONNECT_TO_ME: {
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
		/* Probably ENOMEM error, try again. */
		if (sess == NULL) {
			ddf_msg(LVL_WARN,
			    "Failed to create start input session");
			async_answer_0(icallid, EAGAIN);
			break;
		}
		if (kbd->input_sess == NULL) {
			kbd->input_sess = sess;
			ddf_msg(LVL_DEBUG, "Set input session");
			async_answer_0(icallid, EOK);
		} else {
			ddf_msg(LVL_ERROR, "Input session already set");
			async_answer_0(icallid, ELIMIT);
		}
		break;
	}
	default:
			ddf_msg(LVL_ERROR,
			    "Unknown method: %d.\n", (int)method);
			async_answer_0(icallid, EINVAL);
			break;
	}
}
