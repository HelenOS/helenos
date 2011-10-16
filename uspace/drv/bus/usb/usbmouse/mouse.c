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

#include <usb/debug.h>
#include <errno.h>
#include <str_error.h>
#include <ipc/mouseev.h>
#include <async.h>
#include "mouse.h"

/** Mouse polling callback.
 *
 * @param dev    Device that is being polled.
 * @param buffer Data buffer.
 * @param size   Buffer size in bytes.
 * @param arg    Pointer to usb_mouse_t.
 *
 * @return Always true.
 *
 */
bool usb_mouse_polling_callback(usb_device_t *dev, uint8_t *buffer,
    size_t size, void *arg)
{
	usb_mouse_t *mouse = (usb_mouse_t *) arg;
	
	usb_log_debug2("got buffer: %s.\n",
	    usb_debug_str_buffer(buffer, size, 0));
	
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
	
	if (buffer[1] == 0)
		shift_x = 0;
	
	if (buffer[2] == 0)
		shift_y = 0;
	
	if (buffer[3] == 0)
		wheel = 0;
	
	if (mouse->console_sess) {
		if ((shift_x != 0) || (shift_y != 0)) {
			// FIXME: guessed for QEMU
			
			async_exch_t *exch = async_exchange_begin(mouse->console_sess);
			async_req_2_0(exch, MOUSEEV_MOVE_EVENT, -shift_x / 10, -shift_y / 10);
			async_exchange_end(exch);
		}
		if (butt) {
			// FIXME: proper button clicking
			
			async_exch_t *exch = async_exchange_begin(mouse->console_sess);
			async_req_2_0(exch, MOUSEEV_BUTTON_EVENT, 1, 1);
			async_req_2_0(exch, MOUSEEV_BUTTON_EVENT, 1, 0);
			async_exchange_end(exch);
		}
	}
	
	usb_log_debug("buttons=%s  dX=%+3d  dY=%+3d  wheel=%+3d\n",
	    str_buttons, shift_x, shift_y, wheel);
	
	/* Guess. */
	async_usleep(1000);
	
	return true;
}

/** Callback when polling is terminated.
 *
 * @param dev              Device where the polling terminated.
 * @param recurring_errors Whether the polling was terminated due to
 *                         recurring errors.
 * @param arg              Pointer to usb_mouse_t.
 *
 */
void usb_mouse_polling_ended_callback(usb_device_t *dev, bool recurring_errors,
    void *arg)
{
	usb_mouse_t *mouse = (usb_mouse_t *) arg;
	
	async_hangup(mouse->console_sess);
	mouse->console_sess = NULL;
	
	usb_device_deinit(dev);
}

/**
 * @}
 */
