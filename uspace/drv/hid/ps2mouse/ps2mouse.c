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
/** @addtogroup drvmouse
 * @{
 */
/** @file
 * @brief PS/2 mouse driver.
 */

#include <stdbool.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/log.h>
#include <io/keycode.h>
#include <io/chardev.h>
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

#define Z_SIGN (1 << 3) /* 4th byte */
#define X_SIGN (1 << 4) /* 1st byte */
#define Y_SIGN (1 << 5) /* 1st byte */
#define X_OVERFLOW (1 << 6) /* 1st byte */
#define Y_OVERFLOW (1 << 7) /* 1st byte */

#define BUTTON_LEFT   0
#define BUTTON_RIGHT   1
#define BUTTON_MIDDLE   2
#define PS2_BUTTON_COUNT   3

#define INTELLIMOUSE_ALWAYS_ZERO (0xc0)
#define INTELLIMOUSE_BUTTON_4 (1 << 4) /* 4th byte */
#define INTELLIMOUSE_BUTTON_5 (1 << 5) /* 4th byte */
#define INTELLIMOUSE_BUTTON_COUNT 5

#define PS2_BUTTON_MASK(button) (1 << button)

#define MOUSE_READ_BYTE_TEST(mouse, value_) \
do { \
	uint8_t value = (value_); \
	uint8_t data = 0; \
	size_t nread; \
	const errno_t rc = chardev_read((mouse)->chardev, &data, 1, &nread); \
	if (rc != EOK) { \
		ddf_msg(LVL_ERROR, "Failed reading byte: %s", str_error_name(rc));\
		return rc; \
	} \
	if (data != value) { \
		ddf_msg(LVL_DEBUG, "Failed testing byte: got %hhx vs. %hhx)", \
		    data, value); \
		return EIO; \
	} \
} while (0)

#define MOUSE_WRITE_BYTE(mouse, value_) \
do { \
	uint8_t value = (value_); \
	uint8_t data = (value); \
	size_t nwr; \
	const errno_t rc = chardev_write((mouse)->chardev, &data, 1, &nwr); \
	if (rc != EOK) { \
		ddf_msg(LVL_ERROR, "Failed writing byte: %s", str_error_name(rc)); \
		return rc; \
	} \
} while (0)

static errno_t polling_ps2(void *);
static errno_t polling_intellimouse(void *);
static errno_t probe_intellimouse(ps2_mouse_t *, bool);
static void default_connection_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);

/** ps/2 mouse driver ops. */
static ddf_dev_ops_t mouse_ops = {
	.default_handler = default_connection_handler
};

/** Initialize mouse driver structure.
 *
 * Connects to parent, creates keyboard function, starts polling fibril.
 *
 * @param kbd Mouse driver structure to initialize.
 * @param dev DDF device structure.
 *
 * @return EOK on success or non-zero error code
 */
errno_t ps2_mouse_init(ps2_mouse_t *mouse, ddf_dev_t *dev)
{
	async_sess_t *parent_sess;
	bool bound = false;
	errno_t rc;

	mouse->client_sess = NULL;

	parent_sess = ddf_dev_parent_sess_get(dev);
	if (parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed getting parent session.");
		rc = ENOMEM;
		goto error;
	}

	rc = chardev_open(parent_sess, &mouse->chardev);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed opening character device.");
		goto error;
	}

	mouse->mouse_fun = ddf_fun_create(dev, fun_exposed, "mouse");
	if (mouse->mouse_fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating mouse function.");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(mouse->mouse_fun, &mouse_ops);

	rc = ddf_fun_bind(mouse->mouse_fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding mouse function.");
		goto error;
	}

	bound = true;

	rc = ddf_fun_add_to_category(mouse->mouse_fun, "mouse");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed adding mouse function to category.");
		goto error;
	}

	/* Probe IntelliMouse extensions. */
	errno_t (*polling_f)(void*) = polling_ps2;
	if (probe_intellimouse(mouse, false) == EOK) {
		ddf_msg(LVL_NOTE, "Enabled IntelliMouse extensions");
		polling_f = polling_intellimouse;
		if (probe_intellimouse(mouse, true) == EOK)
			ddf_msg(LVL_NOTE, "Enabled 4th and 5th button.");
	}

	/* Enable mouse data reporting. */
	uint8_t report = PS2_MOUSE_ENABLE_DATA_REPORT;
	size_t nwr;
	rc = chardev_write(mouse->chardev, &report, 1, &nwr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to enable data reporting.");
		rc = EIO;
		goto error;
	}

	size_t nread;
	rc = chardev_read(mouse->chardev, &report, 1, &nread);
	if (rc != EOK || report != PS2_MOUSE_ACK) {
		ddf_msg(LVL_ERROR, "Failed to confirm data reporting: %hhx.",
		    report);
		rc = EIO;
		goto error;
	}

	mouse->polling_fibril = fibril_create(polling_f, mouse);
	if (mouse->polling_fibril == 0) {
		rc = ENOMEM;
		goto error;
	}

	fibril_add_ready(mouse->polling_fibril);
	return EOK;
error:
	if (bound)
		ddf_fun_unbind(mouse->mouse_fun);
	if (mouse->mouse_fun != NULL) {
		ddf_fun_destroy(mouse->mouse_fun);
		mouse->mouse_fun = NULL;
	}

	chardev_close(mouse->chardev);
	mouse->chardev = NULL;
	return rc;
}

/** Read fixed-size mouse packet.
 *
 * Continue reading until entire packet is received.
 *
 * @param mouse Mouse device
 * @param pbuf Buffer for storing packet
 * @param psize Packet size
 *
 * @return EOK on success or non-zero error code
 */
static errno_t ps2_mouse_read_packet(ps2_mouse_t *mouse, void *pbuf, size_t psize)
{
	errno_t rc;
	size_t pos;
	size_t nread;

	pos = 0;
	while (pos < psize) {
		rc = chardev_read(mouse->chardev, pbuf + pos, psize - pos,
		    &nread);
		if (rc != EOK) {
			ddf_msg(LVL_WARN, "Error reading packet.");
			return rc;
		}

		pos += nread;
	}

	return EOK;
}

/** Get data and parse ps2 protocol packets.
 * @param arg Pointer to ps2_mouse_t structure.
 * @return Never.
 */
errno_t polling_ps2(void *arg)
{
	ps2_mouse_t *mouse = (ps2_mouse_t *) arg;
	errno_t rc;

	bool buttons[PS2_BUTTON_COUNT] = {};
	while (1) {
		uint8_t packet[PS2_BUFSIZE] = {};
		rc = ps2_mouse_read_packet(mouse, packet, PS2_BUFSIZE);
		if (rc != EOK)
			continue;

		ddf_msg(LVL_DEBUG2, "Got packet: %hhx:%hhx:%hhx.",
		    packet[0], packet[1], packet[2]);

		async_exch_t *exch =
		    async_exchange_begin(mouse->client_sess);
		if (!exch) {
			ddf_msg(LVL_ERROR,
			    "Failed creating exchange.");
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

	return 0;
}

/** Get data and parse ps2 protocol with IntelliMouse extension packets.
 * @param arg Pointer to ps2_mouse_t structure.
 * @return Never.
 */
static errno_t polling_intellimouse(void *arg)
{
	ps2_mouse_t *mouse = (ps2_mouse_t *) arg;
	errno_t rc;

	bool buttons[INTELLIMOUSE_BUTTON_COUNT] = {};
	while (1) {
		uint8_t packet[INTELLIMOUSE_BUFSIZE] = {};
		rc = ps2_mouse_read_packet(mouse, packet, INTELLIMOUSE_BUFSIZE);
		if (rc != EOK)
			continue;

		ddf_msg(LVL_DEBUG2, "Got packet: %hhx:%hhx:%hhx:%hhx.",
		    packet[0], packet[1], packet[2], packet[3]);

		async_exch_t *exch =
		    async_exchange_begin(mouse->client_sess);
		if (!exch) {
			ddf_msg(LVL_ERROR,
			    "Failed creating exchange.");
			continue;
		}

		/* Buttons */
		/* NOTE: Parsing 4th and 5th button works even if this extension
		 * is not supported and whole 4th byte should be interpreted
		 * as Z-axis movement. the upper 4 bits are just a sign
		 * extension then. + sign is interpreted as "button up"
		 * (i.e no change since that is the default) and - sign fails
		 * the "imb" condition. Thus 4th and 5th buttons are never
		 * down on wheel only extension. */
		const bool imb = (packet[3] & INTELLIMOUSE_ALWAYS_ZERO) == 0;
		const bool status[] = {
			[0] = packet[0] & PS2_BUTTON_MASK(0),
			[1] = packet[0] & PS2_BUTTON_MASK(1),
			[2] = packet[0] & PS2_BUTTON_MASK(2),
			[3] = (packet[3] & INTELLIMOUSE_BUTTON_4) && imb,
			[4] = (packet[3] & INTELLIMOUSE_BUTTON_5) && imb,
		};
		for (unsigned i = 0; i < INTELLIMOUSE_BUTTON_COUNT; ++i) {
			if (buttons[i] != status[i]) {
				buttons[i] = status[i];
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

	return 0;
}

/** Send magic sequence to initialize IntelliMouse extensions.
 * @param exch IPC exchange to the parent device.
 * @param buttons True selects magic sequence for 4th and 5th button,
 * false selects wheel support magic sequence.
 * See http://www.computer-engineering.org/ps2mouse/ for details.
 */
static errno_t probe_intellimouse(ps2_mouse_t *mouse, bool buttons)
{
	MOUSE_WRITE_BYTE(mouse, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(mouse, 200);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(mouse, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(mouse, buttons ? 200 : 100);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(mouse, PS2_MOUSE_SET_SAMPLE_RATE);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);
	MOUSE_WRITE_BYTE(mouse, 80);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);

	MOUSE_WRITE_BYTE(mouse, PS2_MOUSE_GET_DEVICE_ID);
	MOUSE_READ_BYTE_TEST(mouse, PS2_MOUSE_ACK);
	MOUSE_READ_BYTE_TEST(mouse, buttons ? 4 : 3);

	return EOK;
}

/** Default handler for IPC methods not handled by DDF.
 *
 * @param fun Device function handling the call.
 * @param icallid Call id.
 * @param icall Call data.
 */
void default_connection_handler(ddf_fun_t *fun,
    ipc_callid_t icallid, ipc_call_t *icall)
{
	const sysarg_t method = IPC_GET_IMETHOD(*icall);
	ps2_mouse_t *mouse = ddf_dev_data_get(ddf_fun_get_dev(fun));
	async_sess_t *sess;

	switch (method) {
	/* This might be ugly but async_callback_receive_start makes no
	 * difference for incorrect call and malloc failure. */
	case IPC_M_CONNECT_TO_ME:
		sess = async_callback_receive_start(EXCHANGE_SERIALIZE, icall);
		/* Probably ENOMEM error, try again. */
		if (sess == NULL) {
			ddf_msg(LVL_WARN,
			    "Failed creating client callback session");
			async_answer_0(icallid, EAGAIN);
			break;
		}
		if (mouse->client_sess == NULL) {
			mouse->client_sess = sess;
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
