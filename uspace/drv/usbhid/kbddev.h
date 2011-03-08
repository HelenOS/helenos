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

#ifndef USBHID_KBDDEV_H_
#define USBHID_KBDDEV_H_

#include <stdint.h>

#include <usb/classes/hid.h>
#include <ddf/driver.h>
#include <usb/pipes.h>

#include "hiddev.h"

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
typedef struct {
	/** Structure holding generic USB/HID device information. */
	usbhid_dev_t *hid_dev;
	
	/** Currently pressed keys (not translated to key codes). */
	uint8_t *keys;
	/** Count of stored keys (i.e. number of keys in the report). */
	size_t key_count;
	/** Currently pressed modifiers (bitmap). */
	uint8_t modifiers;
	
	/** Currently active modifiers including locks. Sent to the console. */
	unsigned mods;
	
	/** Currently active lock keys. */
	unsigned lock_keys;
	
	/** IPC phone to the console device (for sending key events). */
	int console_phone;
	
	/** State of the structure (for checking before use). */
	int initialized;
} usbhid_kbd_t;

/*----------------------------------------------------------------------------*/

int usbhid_kbd_try_add_device(ddf_dev_t *dev);

#endif /* USBHID_KBDDEV_H_ */

/**
 * @}
 */
