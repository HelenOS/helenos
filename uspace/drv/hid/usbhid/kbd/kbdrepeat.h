/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
