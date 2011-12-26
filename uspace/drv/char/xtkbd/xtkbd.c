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
#include <io/keycode.h>
#include <io/console.h>
#include <ipc/kbdev.h>
#include <abi/ipc/methods.h>

#include "xtkbd.h"
static const int scanmap_simple[] = {

	[0x29] = KC_BACKTICK,

	[0x02] = KC_1,
	[0x03] = KC_2,
	[0x04] = KC_3,
	[0x05] = KC_4,
	[0x06] = KC_5,
	[0x07] = KC_6,
	[0x08] = KC_7,
	[0x09] = KC_8,
	[0x0a] = KC_9,
	[0x0b] = KC_0,

	[0x0c] = KC_MINUS,
	[0x0d] = KC_EQUALS,
	[0x0e] = KC_BACKSPACE,

	[0x0f] = KC_TAB,

	[0x10] = KC_Q,
	[0x11] = KC_W,
	[0x12] = KC_E,
	[0x13] = KC_R,
	[0x14] = KC_T,
	[0x15] = KC_Y,
	[0x16] = KC_U,
	[0x17] = KC_I,
	[0x18] = KC_O,
	[0x19] = KC_P,

	[0x1a] = KC_LBRACKET,
	[0x1b] = KC_RBRACKET,

	[0x3a] = KC_CAPS_LOCK,

	[0x1e] = KC_A,
	[0x1f] = KC_S,
	[0x20] = KC_D,
	[0x21] = KC_F,
	[0x22] = KC_G,
	[0x23] = KC_H,
	[0x24] = KC_J,
	[0x25] = KC_K,
	[0x26] = KC_L,

	[0x27] = KC_SEMICOLON,
	[0x28] = KC_QUOTE,
	[0x2b] = KC_BACKSLASH,

	[0x2a] = KC_LSHIFT,

	[0x2c] = KC_Z,
	[0x2d] = KC_X,
	[0x2e] = KC_C,
	[0x2f] = KC_V,
	[0x30] = KC_B,
	[0x31] = KC_N,
	[0x32] = KC_M,

	[0x33] = KC_COMMA,
	[0x34] = KC_PERIOD,
	[0x35] = KC_SLASH,

	[0x36] = KC_RSHIFT,

	[0x1d] = KC_LCTRL,
	[0x38] = KC_LALT,
	[0x39] = KC_SPACE,

	[0x01] = KC_ESCAPE,

	[0x3b] = KC_F1,
	[0x3c] = KC_F2,
	[0x3d] = KC_F3,
	[0x3e] = KC_F4,
	[0x3f] = KC_F5,
	[0x40] = KC_F6,
	[0x41] = KC_F7,

	[0x42] = KC_F8,
	[0x43] = KC_F9,
	[0x44] = KC_F10,

	[0x57] = KC_F11,
	[0x58] = KC_F12,

	[0x46] = KC_SCROLL_LOCK,

	[0x1c] = KC_ENTER,

	[0x45] = KC_NUM_LOCK,
	[0x37] = KC_NTIMES,
	[0x4a] = KC_NMINUS,
	[0x4e] = KC_NPLUS,
	[0x47] = KC_N7,
	[0x48] = KC_N8,
	[0x49] = KC_N9,
	[0x4b] = KC_N4,
	[0x4c] = KC_N5,
	[0x4d] = KC_N6,
	[0x4f] = KC_N1,
	[0x50] = KC_N2,
	[0x51] = KC_N3,
	[0x52] = KC_N0,
	[0x53] = KC_NPERIOD
};
/*----------------------------------------------------------------------------*/
static const int scanmap_e0[] = {
	[0x38] = KC_RALT,
	[0x1d] = KC_RSHIFT,

	[0x37] = KC_PRTSCR,

	[0x52] = KC_INSERT,
	[0x47] = KC_HOME,
	[0x49] = KC_PAGE_UP,

	[0x53] = KC_DELETE,
	[0x4f] = KC_END,
	[0x51] = KC_PAGE_DOWN,

	[0x48] = KC_UP,
	[0x4b] = KC_LEFT,
	[0x50] = KC_DOWN,
	[0x4d] = KC_RIGHT,

	[0x35] = KC_NSLASH,
	[0x1c] = KC_NENTER
};
/*----------------------------------------------------------------------------*/
static int polling(void *);
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall);
/*----------------------------------------------------------------------------*/
static ddf_dev_ops_t kbd_ops = {
	.default_handler = default_connection_handler
};
/*----------------------------------------------------------------------------*/
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
		const int *map = scanmap_simple;
		size_t map_size = sizeof(scanmap_simple) / sizeof(int);

		uint8_t code = 0;
		ssize_t size = char_dev_read(kbd->parent_sess, &code, 1);

		if (code == 0xe0) {
			map = scanmap_e0;
			map_size = sizeof(scanmap_e0) / sizeof(int);
			size = char_dev_read(kbd->parent_sess, &code, 1);
		}


		kbd_event_type_t type;
		if (code & 0x80) {
			code &= ~0x80;
			type = KEY_RELEASE;
		} else {
			type = KEY_PRESS;
		}

		if ((size_t) code >= map_size) {
			ddf_msg(LVL_WARN,
			    "Unknown scancode: %hhx, size: %zd", code, size);
			continue;
		}

		const unsigned key = map[code];
		if (key != 0) {
			async_exch_t *exch =
			    async_exchange_begin(kbd->input_sess);
			if (!exch) {
				ddf_msg(LVL_ERROR,
				    "Failed to create input exchange.");
				continue;
			}
			async_msg_4(exch, KBDEV_EVENT, type, key, 0, 0);
			async_exchange_end(exch);
		}
	}
}
/*----------------------------------------------------------------------------*/
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	if (fun == NULL || fun->driver_data == NULL) {
		ddf_msg(LVL_ERROR, "%s: Missing parameter.", __FUNCTION__);
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
			ddf_msg(LVL_ERROR, "Unknown method: %d.", (int)method);
			async_answer_0(icallid, EINVAL);
			break;
	}
}
