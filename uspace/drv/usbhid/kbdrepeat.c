/*
 * Copyright (c) 2011 Lubos Slovak
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB HID keyboard autorepeat facilities
 */

#include <fibril_synch.h>
#include <io/keycode.h>
#include <io/console.h>
#include <errno.h>

#include <usb/debug.h>

#include "kbdrepeat.h"
#include "kbddev.h"

static unsigned int CHECK_DELAY = 10000;

/*----------------------------------------------------------------------------*/

static void usbhid_kbd_repeat_loop(usbhid_kbd_t *kbd)
{
	unsigned int delay = 0;
	
	usb_log_debug("Starting autorepeat loop.\n");

	while (true) {
		fibril_mutex_lock(kbd->repeat_mtx);

		if (kbd->repeat.key_new > 0) {
			if (kbd->repeat.key_new == kbd->repeat.key_repeated) {
				usb_log_debug2("Repeating key: %u.\n", 
				    kbd->repeat.key_repeated);
				usbhid_kbd_push_ev(kbd, KEY_PRESS, 
				    kbd->repeat.key_repeated);
				delay = kbd->repeat.delay_between;
			} else {
				usb_log_debug("New key to repeat: %u.\n", 
				    kbd->repeat.key_new);
				kbd->repeat.key_repeated = kbd->repeat.key_new;
				delay = kbd->repeat.delay_before;
			}
		} else {
			if (kbd->repeat.key_repeated > 0) {
				usb_log_debug("Stopping to repeat key: %u.\n", 
				    kbd->repeat.key_repeated);
				kbd->repeat.key_repeated = 0;
			}
			delay = CHECK_DELAY;
		}
		fibril_mutex_unlock(kbd->repeat_mtx);
		
		async_usleep(delay);
	}
}

/*----------------------------------------------------------------------------*/

int usbhid_kbd_repeat_fibril(void *arg)
{
	usb_log_debug("Autorepeat fibril spawned.\n");
	
	if (arg == NULL) {
		usb_log_error("No device!\n");
		return EINVAL;
	}
	
	usbhid_kbd_t *kbd = (usbhid_kbd_t *)arg;
	
	usbhid_kbd_repeat_loop(kbd);
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

void usbhid_kbd_repeat_start(usbhid_kbd_t *kbd, unsigned int key)
{
	fibril_mutex_lock(kbd->repeat_mtx);
	kbd->repeat.key_new = key;
	fibril_mutex_unlock(kbd->repeat_mtx);
}

/*----------------------------------------------------------------------------*/

void usbhid_kbd_repeat_stop(usbhid_kbd_t *kbd, unsigned int key)
{
	fibril_mutex_lock(kbd->repeat_mtx);
	if (key == kbd->repeat.key_new) {
		kbd->repeat.key_new = 0;
	}
	fibril_mutex_unlock(kbd->repeat_mtx);
}

/**
 * @}
 */
