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
/** @addtogroup drvmouse
 * @{
 */
/** @file
 * @brief ps2 mouse driver.
 */

#include <bool.h>
#include <errno.h>
#include <devman.h>
#include <device/char_dev.h>
#include <ddf/log.h>
#include <io/keycode.h>
#include <io/console.h>
#include <ipc/mouseev.h>
#include <abi/ipc/methods.h>

#include "ps2mouse.h"

#define PS2_MOUSE_GET_DEVICE_ID   0xf2
#define PS2_MOUSE_SET_SAMPLE_RATE   0xf3
#define PS2_MOUSE_ENABLE_DATA_REPORT   0xf4
#define PS2_MOUSE_ACK   0xfa

#define PS2_BUFSIZE 3
#define INTELLIMOUSE_BUFSIZE 4

#define Z_SIGN (1 << 3)
#define X_SIGN (1 << 4)
#define Y_SIGN (1 << 5)
#define X_OVERFLOW (1 << 6)
#define Y_OVERFLOW (1 << 7)

#define BUTTON_LEFT   0
#define BUTTON_RIGHT   1
#define BUTTON_MIDDLE   2
#define PS2_BUTTON_COUNT   3
#define INTELLIMOUSE_BUTTON_COUNT 5

#define PS2_BUTTON_MASK(button) (1 << button)

#define MOUSE_READ_BYTE_TEST(sess, value) \
do { \
	uint8_t data = 0; \
	const ssize_t size = char_dev_read(session, &data, 1); \
	if (size != 1) { \
		ddf_msg(LVL_ERROR, "Failed reading byte: %d)", size);\
		return size < 0 ? size : EIO; \
	} \
	if (data != value) { \
		ddf_msg(LVL_ERROR, "Failed testing byte: got %hhx vs. %hhx)", \
		    data, value); \
		return EIO; \
	} \
} while (0)
#define MOUSE_WRITE_BYTE(sess, value) \
do { \
	uint8_t data = value; \
	const ssize_t size = char_dev_write(session, &data, 1); \
	if (size < 0 ) { \
		ddf_msg(LVL_ERROR, "Failed writing byte: %hhx", value); \
		return size; \
	} \
} while (0)
/*----------------------------------------------------------------------------*/
static int polling_ps2(void *);
static int polling_intellimouse(void *);
static int probe_intellimouse(async_sess_t *);
static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);
/*----------------------------------------------------------------------------*/
/** ps/2 mouse driver ops. */
static ddf_dev_ops_t mouse_ops = {
	.default_handler = default_connection_handler
};
/*----------------------------------------------------------------------------*/
/** Initialize mouse driver structure.
 * @param kbd Mouse driver structure to initialize.
 * @param dev DDF device structure.
 *
 * Connects to parent, creates keyboard function, starts polling fibril.
 */
int ps2_mouse_init(ps2_mouse_t *mouse, ddf_dev_t *dev)
{
	assert(mouse);
	assert(dev);
	mouse->input_sess = NULL;
	mouse->parent_sess = devman_parent_device_connect(EXCHANGE_SERIALIZE,
	    dev->handle, IPC_FLAG_BLOCKING);
	if (!mouse->parent_sess)
		return ENOMEM;

	mouse->mouse_fun = ddf_fun_create(dev, fun_exposed, "mouse");
	if (!mouse->mouse_fun) {
		async_hangup(mouse->parent_sess);
		return ENOMEM;
	}
	mouse->mouse_fun->ops = &mouse_ops;
	mouse->mouse_fun->driver_data = mouse;

	int ret = ddf_fun_bind(mouse->mouse_fun);
	if (ret != EOK) {
		async_hangup(mouse->parent_sess);
		mouse->mouse_fun->driver_data = NULL;
		ddf_fun_destroy(mouse->mouse_fun);
		return ENOMEM;
	}

	ret = ddf_fun_add_to_category(mouse->mouse_fun, "mouse");
	if (ret != EOK) {
		async_hangup(mouse->parent_sess);
		ddf_fun_unbind(mouse->mouse_fun);
		mouse->mouse_fun->driver_data = NULL;
		ddf_fun_destroy(mouse->mouse_fun);
		return ENOMEM;
	}
	/* Probe IntelliMouse extensions. */
	int (*polling_f)(void*) = polling_ps2;
	if (probe_intellimouse(mouse->parent_sess) == EOK) {
		ddf_msg(LVL_NOTE, "Enabled IntelliMouse extensions");
		polling_f = polling_intellimouse;
	}
	/* Enable mouse data reporting. */
	uint8_t report = PS2_MOUSE_ENABLE_DATA_REPORT;
	ssize_t size = char_dev_write(mouse->parent_sess, &report, 1);
	if (size != 1) {
		ddf_msg(LVL_ERROR, "Failed to enable data reporting.");
		async_hangup(mouse->parent_sess);
		ddf_fun_unbind(mouse->mouse_fun);
		mouse->mouse_fun->driver_data = NULL;
		ddf_fun_destroy(mouse->mouse_fun);
		return EIO;
	}

	size = char_dev_read(mouse->parent_sess, &report, 1);
	if (size != 1 || report != PS2_MOUSE_ACK) {
		ddf_msg(LVL_ERROR, "Failed to confirm data reporting: %hhx.",
		    report);
		async_hangup(mouse->parent_sess);
		ddf_fun_unbind(mouse->mouse_fun);
		mouse->mouse_fun->driver_data = NULL;
		ddf_fun_destroy(mouse->mouse_fun);
		return EIO;
	}

	mouse->polling_fibril = fibril_create(polling_f, mouse);
	if (!mouse->polling_fibril) {
		async_hangup(mouse->parent_sess);
		ddf_fun_unbind(mouse->mouse_fun);
		mouse->mouse_fun->driver_data = NULL;
		ddf_fun_destroy(mouse->mouse_fun);
		return ENOMEM;
	}
	fibril_add_ready(mouse->polling_fibril);
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Get data and parse ps2 protocol packets.
 * @param arg Pointer to ps2_mouse_t structure.
 * @return Never.
 */
int polling_ps2(void *arg)
{
	assert(arg);
	const ps2_mouse_t *mouse = arg;

	assert(mouse->parent_sess);
	bool buttons[PS2_BUTTON_COUNT] = {};
	while (1) {

		uint8_t packet[PS2_BUFSIZE] = {};
		const ssize_t size =
		    char_dev_read(mouse->parent_sess, packet, PS2_BUFSIZE);

		if (size != PS2_BUFSIZE) {
			ddf_msg(LVL_WARN, "Incorrect packet size: %zd.", size);
			continue;
		}
		ddf_msg(LVL_DEBUG2, "Got packet: %hhx:%hhx:%hhx.",
		    packet[0], packet[1], packet[2]);

		async_exch_t *exch =
		    async_exchange_begin(mouse->input_sess);
		if (!exch) {
			ddf_msg(LVL_ERROR,
			    "Failed to create input exchange.");
			continue;
		}

		/* Buttons */
		for (unsigned i = 0; i < PS2_BUTTON_COUNT; ++i) {
			const bool status = (packet[0] & PS2_BUTTON_MASK(i));
			if (buttons[i] != status) {
				buttons[i] = status;
				async_msg_2(exch, MOUSEEV_BUTTON_EVENT, i + 1,
				    buttons[i]);
			}
		}

		/* Movement */
		const int16_t move_x =
		    ((packet[0] & X_SIGN) ? 0xff00 : 0) | packet[1];
		const int16_t move_y =
		    (((packet[0] & Y_SIGN) ? 0xff00 : 0) | packet[2]);
		//TODO: Consider overflow bit
		if (move_x != 0 || move_y != 0) {
			async_msg_2(exch, MOUSEEV_MOVE_EVENT, move_x, -move_y);
		}
		async_exchange_end(exch);
	}
}
/*----------------------------------------------------------------------------*/
/** Get data and parse ps2 protocol with intellimouse extension packets.
 * @param arg Pointer to ps2_mouse_t structure.
 * @return Never.
 */
static int polling_intellimouse(void *arg)
{
	assert(arg);
	const ps2_mouse_t *mouse = arg;

	assert(mouse->parent_sess);
	bool buttons[INTELLIMOUSE_BUTTON_COUNT] = {};
	while (1) {

		uint8_t packet[INTELLIMOUSE_BUFSIZE] = {};
		const ssize_t size = char_dev_read(
		    mouse->parent_sess, packet, INTELLIMOUSE_BUFSIZE);

		if (size != INTELLIMOUSE_BUFSIZE) {
			ddf_msg(LVL_WARN, "Incorrect packet size: %zd.", size);
			continue;
		}
		ddf_msg(LVL_DEBUG2, "Got packet: %hhx:%hhx:%hhx:%hhx.",
		    packet[0], packet[1], packet[2], packet[3]);

		async_exch_t *exch =
		    async_exchange_begin(mouse->input_sess);
		if (!exch) {
			ddf_msg(LVL_ERROR,
			    "Failed to create input exchange.");
			continue;
		}
		/* ps/2 Buttons */
		for (unsigned i = 0; i < PS2_BUTTON_COUNT; ++i) {
			const bool status = (packet[0] & PS2_BUTTON_MASK(i));
			if (buttons[i] != status) {
				buttons[i] = status;
				async_msg_2(exch, MOUSEEV_BUTTON_EVENT, i + 1,
				    buttons[i]);
			}
		}

		/* Movement */
		const int16_t move_x =
		    ((packet[0] & X_SIGN) ? 0xff00 : 0) | packet[1];
		const int16_t move_y =
		    (((packet[0] & Y_SIGN) ? 0xff00 : 0) | packet[2]);
		const int8_t move_z =
		    (((packet[3] & Z_SIGN) ? 0xf0 : 0) | (packet[3] & 0xf));
		ddf_msg(LVL_DEBUG2, "Parsed moves: %d:%d:%hhd", move_x, move_y,
		    move_z);
		//TODO: Consider overflow bit
		if (move_x != 0 || move_y != 0 || move_z != 0) {
			async_msg_3(exch, MOUSEEV_MOVE_EVENT,
			    move_x, -move_y, -move_z);
		}
		async_exchange_end(exch);
	}
}
/*----------------------------------------------------------------------------*/
static int probe_intellimouse(async_sess_t *session)
{
	assert(session);

	MOUSE_WRITE_BYTE(session, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(session, 200);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(session, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(session, 100);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(session, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(session, 80);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(session, PS2_MOUSE_GET_DEVICE_ID);
	MOUSE_READ_BYTE_TEST(session, PS2_MOUSE_ACK);
	MOUSE_READ_BYTE_TEST(session, 3);

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun Device function handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	if (fun == NULL || fun->driver_data == NULL) {
		ddf_msg(LVL_ERROR, "%s: Missing parameter.", __FUNCTION__);
		async_answer_0(icallid, EINVAL);
		return;
	}

	const sysarg_t method = IPC_GET_IMETHOD(*icall);
	ps2_mouse_t *mouse = fun->driver_data;

	switch (method) {
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
		if (mouse->input_sess == NULL) {
			mouse->input_sess = sess;
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
