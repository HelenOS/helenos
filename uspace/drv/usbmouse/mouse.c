/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup drvusbmouse
 * @{
 */
/**
 * @file
 * Actual handling of USB mouse protocol.
 */
#include "mouse.h"
#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/mouse.h>

int usb_mouse_polling_fibril(void *arg)
{
	assert(arg != NULL);
	ddf_dev_t *dev = (ddf_dev_t *) arg;
	usb_mouse_t *mouse = (usb_mouse_t *) dev->driver_data;

	assert(mouse);

	size_t buffer_size = mouse->poll_pipe.max_packet_size;

	if (buffer_size < 4) {
		usb_log_error("Weird mouse, results will be skewed.\n");
		buffer_size = 4;
	}

	uint8_t *buffer = malloc(buffer_size);
	if (buffer == NULL) {
		usb_log_error("Out of memory, poll fibril aborted.\n");
		return ENOMEM;
	}

	while (true) {
		async_usleep(mouse->poll_interval_us);

		size_t actual_size;
		int rc;

		/*
		 * Error checking note:
		 * - failure when starting a session is considered
		 *   temporary (e.g. out of phones, next try might succeed)
		 * - failure of transfer considered fatal (probably the
		 *   device was unplugged)
		 * - session closing not checked (shall not fail anyway)
		 */

		rc = usb_endpoint_pipe_start_session(&mouse->poll_pipe);
		if (rc != EOK) {
			usb_log_warning("Failed to start session, will try again: %s.\n",
			    str_error(rc));
			continue;
		}

		rc = usb_endpoint_pipe_read(&mouse->poll_pipe,
		    buffer, buffer_size, &actual_size);

		usb_endpoint_pipe_end_session(&mouse->poll_pipe);

		if (rc != EOK) {
			usb_log_error("Failed reading mouse input: %s.\n",
			    str_error(rc));
			break;
		}

		usb_log_debug2("got buffer: %s.\n",
		    usb_debug_str_buffer(buffer, buffer_size, 0));

		uint8_t butt = buffer[0];
		char str_buttons[4] = {
			butt & 1 ? '#' : '.',
			butt & 2 ? '#' : '.',
			butt & 4 ? '#' : '.',
			0
		};

		int shift_x = ((int) buffer[1]) - 127;
		int shift_y = ((int) buffer[2]) - 127;
		int wheel = ((int) buffer[3]) - 127;

		if (buffer[1] == 0) {
			shift_x = 0;
		}
		if (buffer[2] == 0) {
			shift_y = 0;
		}
		if (buffer[3] == 0) {
			wheel = 0;
		}

		if (mouse->console_phone >= 0) {
			if ((shift_x != 0) || (shift_y != 0)) {
				/* FIXME: guessed for QEMU */
				async_req_2_0(mouse->console_phone,
				    MEVENT_MOVE,
				    - shift_x / 10,  - shift_y / 10);
			}
			if (butt) {
				/* FIXME: proper button clicking. */
				async_req_2_0(mouse->console_phone,
				    MEVENT_BUTTON, 1, 1);
				async_req_2_0(mouse->console_phone,
				    MEVENT_BUTTON, 1, 0);
			}
		}

		usb_log_debug("buttons=%s  dX=%+3d  dY=%+3d  wheel=%+3d\n",
		   str_buttons, shift_x, shift_y, wheel);
	}

	/*
	 * Device was probably unplugged.
	 * Hang-up the phone to the console.
	 * FIXME: release allocated memory.
	 */
	async_hangup(mouse->console_phone);
	mouse->console_phone = -1;

	usb_log_error("Mouse polling fibril terminated.\n");

	return EOK;
}


/**
 * @}
 */
