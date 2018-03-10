/*
 * Copyright (c) 2017 Jiri Svoboda
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2009 Vineeth Pillai
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
 * @brief AT keyboard driver
 */

#include <errno.h>
#include <ddf/log.h>
#include <io/keycode.h>
#include <io/chardev.h>
#include <io/console.h>
#include <ipc/kbdev.h>
#include <abi/ipc/methods.h>
#include "atkbd.h"

#define AT_CAPS_SCAN_CODE 0x58
#define AT_NUM_SCAN_CODE 0x77
#define AT_SCROLL_SCAN_CODE 0x7E

/* Set 2 scan codes (AT keyboard) */
static const unsigned int scanmap_simple[] = {
	[0x0e] = KC_BACKTICK,

	[0x16] = KC_1,
	[0x1e] = KC_2,
	[0x26] = KC_3,
	[0x25] = KC_4,
	[0x2e] = KC_5,
	[0x36] = KC_6,
	[0x3d] = KC_7,
	[0x3e] = KC_8,
	[0x46] = KC_9,
	[0x45] = KC_0,

	[0x4e] = KC_MINUS,
	[0x55] = KC_EQUALS,
	[0x66] = KC_BACKSPACE,

	[0x0d] = KC_TAB,

	[0x15] = KC_Q,
	[0x1d] = KC_W,
	[0x24] = KC_E,
	[0x2d] = KC_R,
	[0x2c] = KC_T,
	[0x35] = KC_Y,
	[0x3c] = KC_U,
	[0x43] = KC_I,
	[0x44] = KC_O,
	[0x4d] = KC_P,

	[0x54] = KC_LBRACKET,
	[0x5b] = KC_RBRACKET,

	[0x58] = KC_CAPS_LOCK,

	[0x1c] = KC_A,
	[0x1b] = KC_S,
	[0x23] = KC_D,
	[0x2b] = KC_F,
	[0x34] = KC_G,
	[0x33] = KC_H,
	[0x3b] = KC_J,
	[0x42] = KC_K,
	[0x4b] = KC_L,

	[0x4c] = KC_SEMICOLON,
	[0x52] = KC_QUOTE,
	[0x5d] = KC_BACKSLASH,

	[0x12] = KC_LSHIFT,

	[0x1a] = KC_Z,
	[0x22] = KC_X,
	[0x21] = KC_C,
	[0x2a] = KC_V,
	[0x32] = KC_B,
	[0x31] = KC_N,
	[0x3a] = KC_M,

	[0x41] = KC_COMMA,
	[0x49] = KC_PERIOD,
	[0x4a] = KC_SLASH,

	[0x59] = KC_RSHIFT,

	[0x14] = KC_LCTRL,
	[0x11] = KC_LALT,
	[0x29] = KC_SPACE,

	[0x76] = KC_ESCAPE,

	[0x05] = KC_F1,
	[0x06] = KC_F2,
	[0x04] = KC_F3,
	[0x0c] = KC_F4,
	[0x03] = KC_F5,
	[0x0b] = KC_F6,
	[0x02] = KC_F7,

	[0x0a] = KC_F8,
	[0x01] = KC_F9,
	[0x09] = KC_F10,

	[0x78] = KC_F11,
	[0x07] = KC_F12,

	[0x7e] = KC_SCROLL_LOCK,

	[0x5a] = KC_ENTER,

	[0x77] = KC_NUM_LOCK,
	[0x7c] = KC_NTIMES,
	[0x7b] = KC_NMINUS,
	[0x79] = KC_NPLUS,
	[0x6c] = KC_N7,
	[0x75] = KC_N8,
	[0x7d] = KC_N9,
	[0x6b] = KC_N4,
	[0x73] = KC_N5,
	[0x74] = KC_N6,
	[0x69] = KC_N1,
	[0x72] = KC_N2,
	[0x7a] = KC_N3,
	[0x70] = KC_N0,
	[0x71] = KC_NPERIOD,
};

#define KBD_SCANCODE_SET_EXTENDED		0xe0
#define KBD_SCANCODE_SET_EXTENDED_SPECIAL	0xe1
#define KBD_SCANCODE_KEY_RELEASE		0xf0

static const unsigned int scanmap_e0[] = {
	[0x65] = KC_RALT,
	[0x59] = KC_RSHIFT,

	[0x64] = KC_PRTSCR,

	[0x70] = KC_INSERT,
	[0x6c] = KC_HOME,
	[0x7d] = KC_PAGE_UP,

	[0x71] = KC_DELETE,
	[0x69] = KC_END,
	[0x7a] = KC_PAGE_DOWN,

	[0x75] = KC_UP,
	[0x6b] = KC_LEFT,
	[0x72] = KC_DOWN,
	[0x74] = KC_RIGHT,

	[0x4a] = KC_NSLASH,
	[0x5a] = KC_NENTER
};

static void push_event(async_sess_t *sess, kbd_event_type_t type,
    unsigned int key)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_4(exch, KBDEV_EVENT, type, key, 0, 0);
	async_exchange_end(exch);
}

/** Get data and parse scancodes.
 *
 * @param arg Pointer to at_kbd_t structure.
 *
 * @return EIO on error.
 *
 */
static errno_t polling(void *arg)
{
	at_kbd_t *kbd = arg;
	size_t nwr;
	errno_t rc;

	while (true) {
		uint8_t code = 0;
		rc = chardev_read(kbd->chardev, &code, 1, &nwr);
		if (rc != EOK)
			return EIO;

		const unsigned int *map;
		size_t map_size;

		if (code == KBD_SCANCODE_SET_EXTENDED) {
			map = scanmap_e0;
			map_size = sizeof(scanmap_e0) / sizeof(unsigned int);

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
		} else if (code == KBD_SCANCODE_SET_EXTENDED_SPECIAL) {
			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0x14)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0x77)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0xe1)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0xf0)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0x14)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code != 0xf0)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
			if (code == 0x77)
				push_event(kbd->client_sess, KEY_PRESS, KC_BREAK);

			continue;
		} else {
			map = scanmap_simple;
			map_size = sizeof(scanmap_simple) / sizeof(unsigned int);
		}

		kbd_event_type_t type;
		if (code == KBD_SCANCODE_KEY_RELEASE) {
			type = KEY_RELEASE;
			rc = chardev_read(kbd->chardev, &code, 1, &nwr);
			if (rc != EOK)
				return EIO;
		} else {
			type = KEY_PRESS;
		}

		const unsigned int key = (code < map_size) ? map[code] : 0;

		if (key != 0)
			push_event(kbd->client_sess, type, key);
		else
			ddf_msg(LVL_WARN, "Unknown scancode: %hhx", code);
	}
}

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun     Device function handling the call.
 * @param icallid Call id.
 * @param icall   Call data.
 *
 */
static void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	const sysarg_t method = IPC_GET_IMETHOD(*icall);
	at_kbd_t *kbd = ddf_dev_data_get(ddf_fun_get_dev(fun));
	async_sess_t *sess;

	switch (method) {
	case KBDEV_SET_IND:
		async_answer_0(icallid, ENOTSUP);
		break;
	/*
	 * This might be ugly but async_callback_receive_start makes no
	 * difference for incorrect call and malloc failure.
	 */
	case IPC_M_CONNECT_TO_ME:
		sess = async_callback_receive_start(EXCHANGE_SERIALIZE, icall);

		/* Probably ENOMEM error, try again. */
		if (sess == NULL) {
			ddf_msg(LVL_WARN,
			    "Failed creating callback session");
			async_answer_0(icallid, EAGAIN);
			break;
		}

		if (kbd->client_sess == NULL) {
			kbd->client_sess = sess;
			ddf_msg(LVL_DEBUG, "Set client session");
			async_answer_0(icallid, EOK);
		} else {
			ddf_msg(LVL_ERROR, "Client session already set");
			async_answer_0(icallid, ELIMIT);
		}

		break;
	default:
		ddf_msg(LVL_ERROR, "Unknown method: %d.", (int)method);
		async_answer_0(icallid, EINVAL);
		break;
	}
}

/** Keyboard function ops. */
static ddf_dev_ops_t kbd_ops = {
	.default_handler = default_connection_handler
};

/** Initialize keyboard driver structure.
 *
 * @param kbd Keyboard driver structure to initialize.
 * @param dev DDF device structure.
 *
 * Connects to parent, creates keyboard function, starts polling fibril.
 *
 */
errno_t at_kbd_init(at_kbd_t *kbd, ddf_dev_t *dev)
{
	async_sess_t *parent_sess;
	errno_t rc;

	assert(kbd);
	assert(dev);

	kbd->client_sess = NULL;
	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating parent session.");
		rc = EIO;
		goto error;
	}

	rc = chardev_open(parent_sess, &kbd->chardev);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed opening character device.");
		return EIO;
	}

	kbd->kbd_fun = ddf_fun_create(dev, fun_exposed, "kbd");
	if (!kbd->kbd_fun) {
		ddf_msg(LVL_ERROR, "Failed creating function 'kbd'.");
		return ENOMEM;
	}

	ddf_fun_set_ops(kbd->kbd_fun, &kbd_ops);

	errno_t ret = ddf_fun_bind(kbd->kbd_fun);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'kbd'.");
		ddf_fun_destroy(kbd->kbd_fun);
		return EEXIST;
	}

	ret = ddf_fun_add_to_category(kbd->kbd_fun, "keyboard");
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function 'kbd' to category "
		    "'keyboard'.");
		ddf_fun_unbind(kbd->kbd_fun);
		ddf_fun_destroy(kbd->kbd_fun);
		return ENOMEM;
	}

	kbd->polling_fibril = fibril_create(polling, kbd);
	if (!kbd->polling_fibril) {
		ddf_msg(LVL_ERROR, "Failed creating polling fibril.");
		ddf_fun_unbind(kbd->kbd_fun);
		ddf_fun_destroy(kbd->kbd_fun);
		return ENOMEM;
	}

	fibril_add_ready(kbd->polling_fibril);
	return EOK;
error:
	chardev_close(kbd->chardev);
	kbd->chardev = NULL;
	return rc;
}
