/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB blink(1) subdriver.
 */

#ifndef USB_HID_BLINK1_H_
#define USB_HID_BLINK1_H_

#include <usb/dev/driver.h>
#include "../usbhid.h"

/** Container for USB blink(1) device. */
typedef struct {
	/** DDF blink(1) function */
	ddf_fun_t *fun;

	/** USB HID device */
	usb_hid_dev_t *hid_dev;
} usb_blink1_t;

extern const char *HID_BLINK1_FUN_NAME;
extern const char *HID_BLINK1_CATEGORY;

extern errno_t usb_blink1_init(usb_hid_dev_t *, void **);
extern void usb_blink1_deinit(usb_hid_dev_t *, void *);

#endif // USB_HID_BLINK1_H_

/**
 * @}
 */
