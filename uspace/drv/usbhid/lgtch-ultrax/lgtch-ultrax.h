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
 * USB Logitech UltraX Keyboard sample driver.
 */

#ifndef USB_HID_LGTCH_ULTRAX_H_
#define USB_HID_LGTCH_ULTRAX_H_

#include <usb/devdrv.h>

struct usb_hid_dev;

/*----------------------------------------------------------------------------*/
/**
 * USB/HID keyboard device type.
 *
 * Holds a reference to generic USB/HID device structure and keyboard-specific
 * data, such as currently pressed keys, modifiers and lock keys.
 *
 * Also holds a IPC phone to the console (since there is now no other way to 
 * communicate with it).
 *
 * @note Storing active lock keys in this structure results in their setting
 *       being device-specific.
 */
typedef struct usb_lgtch_ultrax_t {
	/** Previously pressed keys (not translated to key codes). */
	int32_t *keys_old;
	/** Currently pressed keys (not translated to key codes). */
	int32_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	size_t key_count;
	
	/** IPC phone to the console device (for sending key events). */
	int console_phone;

	/** Information for auto-repeat of keys. */
//	usb_kbd_repeat_t repeat;
	
	/** Mutex for accessing the information about auto-repeat. */
//	fibril_mutex_t *repeat_mtx;

	/** State of the structure (for checking before use). 
	 * 
	 * 0 - not initialized
	 * 1 - initialized
	 * -1 - ready for destroying
	 */
	int initialized;
} usb_lgtch_ultrax_t;

/*----------------------------------------------------------------------------*/

int usb_lgtch_init(struct usb_hid_dev *hid_dev);

void usb_lgtch_deinit(struct usb_hid_dev *hid_dev);

bool usb_lgtch_polling_callback(struct usb_hid_dev *hid_dev, uint8_t *buffer,
    size_t buffer_size);

/*----------------------------------------------------------------------------*/

#endif // USB_HID_LGTCH_ULTRAX_H_

/**
 * @}
 */
