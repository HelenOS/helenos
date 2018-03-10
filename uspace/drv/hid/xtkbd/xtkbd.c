/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Jiri Svoboda
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
 * @brief XT keyboard driver
 */

#include <errno.h>
#include <ddf/log.h>
#include <io/keycode.h>
#include <io/chardev.h>
#include <io/console.h>
#include <ipc/kbdev.h>
#include <abi/ipc/methods.h>
#include "xtkbd.h"

/** Scancode set 1 table. */
static const unsigned int scanmap_simple[] = {
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

#define KBD_ACK  0xfa
#define KBD_RESEND  0xfe
#define KBD_SCANCODE_SET_EXTENDED  0xe0
#define KBD_SCANCODE_SET_EXTENDED_SPECIAL  0xe1

/** Scancode set 1 extended codes table */
static const unsigned int scanmap_e0[] = {
	[0x38] = KC_RALT,
	[0x1d] = KC_RCTRL,

	[0x37] = KC_SYSREQ,

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

#define KBD_CMD_SET_LEDS  0xed

enum led_indicators {
	LI_SCROLL = 0x01,
	LI_NUM    = 0x02,
	LI_CAPS   = 0x04
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
 * @param arg Pointer to xt_kbd_t structure.
 *
 * @return EIO on error.
 *
 */
static errno_t polling(void *arg)
{
	xt_kbd_t *kbd = arg;
	size_t nread;
	errno_t rc;

	while (true) {
		const unsigned int *map = scanmap_simple;
		size_t map_size = sizeof(scanmap_simple) / sizeof(unsigned int);

		uint8_t code = 0;
		rc = chardev_read(kbd->chardev, &code, 1, &nread);
		if (rc != EOK)
			return EIO;

		/* Ignore AT command reply */
		if ((code == KBD_ACK) || (code == KBD_RESEND))
			continue;

		/* Extended set */
		if (code == KBD_SCANCODE_SET_EXTENDED) {
			map = scanmap_e0;
			map_size = sizeof(scanmap_e0) / sizeof(unsigned int);

			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			/* Handle really special keys */

			if (code == 0x2a) {  /* Print Screen */
				rc = chardev_read(kbd->chardev, &code, 1, &nread);
				if (rc != EOK)
					return EIO;

				if (code != 0xe0)
					continue;

				rc = chardev_read(kbd->chardev, &code, 1, &nread);
				if (rc != EOK)
					return EIO;

				if (code == 0x37)
					push_event(kbd->client_sess, KEY_PRESS, KC_PRTSCR);

				continue;
			}

			if (code == 0x46) {  /* Break */
				rc = chardev_read(kbd->chardev, &code, 1, &nread);
				if (rc != EOK)
					return EIO;

				if (code != 0xe0)
					continue;

				rc = chardev_read(kbd->chardev, &code, 1, &nread);
				if (rc != EOK)
					return EIO;

				if (code == 0xc6)
					push_event(kbd->client_sess, KEY_PRESS, KC_BREAK);

				continue;
			}
		}

		/* Extended special set */
		if (code == KBD_SCANCODE_SET_EXTENDED_SPECIAL) {
			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			if (code != 0x1d)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			if (code != 0x45)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			if (code != 0xe1)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			if (code != 0x9d)
				continue;

			rc = chardev_read(kbd->chardev, &code, 1, &nread);
			if (rc != EOK)
				return EIO;

			if (code == 0xc5)
				push_event(kbd->client_sess, KEY_PRESS, KC_PAUSE);

			continue;
		}

		/* Bit 7 indicates press/release */
		const kbd_event_type_t type =
		    (code & 0x80) ? KEY_RELEASE : KEY_PRESS;
		code &= ~0x80;

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
	xt_kbd_t *kbd = ddf_dev_data_get(ddf_fun_get_dev(fun));
	unsigned mods;
	async_sess_t *sess;

	switch (method) {
	case KBDEV_SET_IND:
		/*
		 * XT keyboards do not support setting mods,
		 * assume AT keyboard with Scan Code Set 1.
		 */
		mods = IPC_GET_ARG1(*icall);
		const uint8_t status = 0 |
		    ((mods & KM_CAPS_LOCK) ? LI_CAPS : 0) |
		    ((mods & KM_NUM_LOCK) ? LI_NUM : 0) |
		    ((mods & KM_SCROLL_LOCK) ? LI_SCROLL : 0);
		uint8_t cmds[] = { KBD_CMD_SET_LEDS, status };

		size_t nwr;
		errno_t rc = chardev_write(kbd->chardev, &cmds[0], 1, &nwr);
		if (rc != EOK) {
			async_answer_0(icallid, rc);
			break;
		}

		rc = chardev_write(kbd->chardev, &cmds[1], 1, &nwr);
		async_answer_0(icallid, rc);
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
errno_t xt_kbd_init(xt_kbd_t *kbd, ddf_dev_t *dev)
{
	async_sess_t *parent_sess;
	bool bound = false;
	errno_t rc;

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
		goto error;
	}

	kbd->kbd_fun = ddf_fun_create(dev, fun_exposed, "kbd");
	if (kbd->kbd_fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function 'kbd'.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(kbd->kbd_fun, &kbd_ops);

	rc = ddf_fun_bind(kbd->kbd_fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function 'kbd'.");
		goto error;
	}

	rc = ddf_fun_add_to_category(kbd->kbd_fun, "keyboard");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding function 'kbd' to category "
		    "'keyboard'.");
		goto error;
	}

	kbd->polling_fibril = fibril_create(polling, kbd);
	if (kbd->polling_fibril == 0) {
		ddf_msg(LVL_ERROR, "Failed creating polling fibril.");
		rc = ENOMEM;
		goto error;
	}

	fibril_add_ready(kbd->polling_fibril);
	return EOK;
error:
	if (bound)
		ddf_fun_unbind(kbd->kbd_fun);
	if (kbd->kbd_fun != NULL)
		ddf_fun_destroy(kbd->kbd_fun);
	chardev_close(kbd->chardev);
	return rc;
}
