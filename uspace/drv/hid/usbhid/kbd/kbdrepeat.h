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
 * USB HID keyboard autorepeat facilities
 */

#ifndef USB_HID_KBDREPEAT_H_
#define USB_HID_KBDREPEAT_H_

/** Delay between auto-repeat state checks when no key is being repeated. */
#define CHECK_DELAY 10000

struct usb_kbd_t;

/**
 * Structure for keeping information needed for auto-repeat of keys.
 */
typedef struct {
	/** Last pressed key. */
	unsigned int key_new;
	/** Key to be repeated. */
	unsigned int key_repeated;
	/** Delay before first repeat in microseconds. */
	unsigned int delay_before;
	/** Delay between repeats in microseconds. */
	unsigned int delay_between;
} usb_kbd_repeat_t;

errno_t usb_kbd_repeat_fibril(void *arg);

void usb_kbd_repeat_start(struct usb_kbd_t *kbd, unsigned int key);

void usb_kbd_repeat_stop(struct usb_kbd_t *kbd, unsigned int key);

#endif /* USB_HID_KBDREPEAT_H_ */

/**
 * @}
 */
