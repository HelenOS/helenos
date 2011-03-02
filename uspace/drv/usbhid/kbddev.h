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
 * @brief USB/HID keyboard device type.
 */
typedef struct {
	usbhid_dev_t *hid_dev;
	
	uint8_t *keycodes;
	size_t keycode_count;
	uint8_t modifiers;
	
	unsigned mods;
	unsigned lock_keys;
	
	int console_phone;
	
	int initialized;
} usbhid_kbd_t;

/*----------------------------------------------------------------------------*/

int usbhid_kbd_try_add_device(ddf_dev_t *dev);

#endif /* USBHID_KBDDEV_H_ */

/**
 * @}
 */
