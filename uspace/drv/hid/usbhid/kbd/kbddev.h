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
/** @file
 * USB HID keyboard device structure and API.
 */

#ifndef USB_HID_KBDDEV_H_
#define USB_HID_KBDDEV_H_

#include <stdint.h>
#include <async.h>
#include <fibril_synch.h>
#include <usb/hid/hid.h>
#include <usb/hid/hidparser.h>
#include <ddf/driver.h>
#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>
#include "../usbhid.h"
#include "kbdrepeat.h"

/**
 * USB/HID keyboard device type.
 *
 * Holds a reference to generic USB/HID device structure and keyboard-specific
 * data, such as currently pressed keys, modifiers and lock keys.
 *
 * Also holds a IPC session to the console (since there is now no other way to
 * communicate with it).
 *
 * @note Storing active lock keys in this structure results in their setting
 *       being device-specific.
 */
typedef struct usb_kbd_t {
	/** Link to HID device structure */
	usb_hid_dev_t *hid_dev;

	/** Previously pressed keys (not translated to key codes). */
	int32_t *keys_old;
	/** Currently pressed keys (not translated to key codes). */
	int32_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	size_t key_count;
	/** Currently pressed modifiers (bitmap). */
	uint8_t modifiers;

	/** Currently active modifiers including locks. Sent to the console. */
	unsigned int mods;

	/** Currently active lock keys. */
	unsigned int lock_keys;

	/** IPC session to client (for sending key events). */
	async_sess_t *client_sess;

	/** Information for auto-repeat of keys. */
	usb_kbd_repeat_t repeat;

	/** Mutex for accessing the information about auto-repeat. */
	fibril_mutex_t repeat_mtx;

	uint8_t *output_buffer;

	size_t output_size;

	size_t led_output_size;

	usb_hid_report_path_t *led_path;

	int32_t *led_data;

	/** State of the structure (for checking before use).
	 *
	 * 0 - not initialized
	 * 1 - initialized
	 * -1 - ready for destroying
	 */
	int initialized;

	/** DDF function */
	ddf_fun_t *fun;
} usb_kbd_t;

extern const usb_endpoint_description_t usb_hid_kbd_poll_endpoint_description;

extern const char *HID_KBD_FUN_NAME;
extern const char *HID_KBD_CATEGORY;

extern errno_t usb_kbd_init(usb_hid_dev_t *, void **);
extern bool usb_kbd_polling_callback(usb_hid_dev_t *, void *);
extern int usb_kbd_is_initialized(const usb_kbd_t *);
extern int usb_kbd_is_ready_to_destroy(const usb_kbd_t *);
extern void usb_kbd_destroy(usb_kbd_t *);
extern void usb_kbd_push_ev(usb_kbd_t *, int, unsigned int);
extern void usb_kbd_deinit(usb_hid_dev_t *, void *);
extern errno_t usb_kbd_set_boot_protocol(usb_hid_dev_t *);

#endif /* USB_HID_KBDDEV_H_ */

/**
 * @}
 */
