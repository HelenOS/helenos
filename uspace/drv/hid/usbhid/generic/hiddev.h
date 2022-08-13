/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB HID driver API.
 */

#ifndef USB_HID_HIDDDEV_H_
#define USB_HID_HIDDDEV_H_

#include <usb/dev/driver.h>
#include "../usbhid.h"

extern const usb_endpoint_description_t
    usb_hid_generic_poll_endpoint_description;

extern const char *HID_GENERIC_FUN_NAME;
extern const char *HID_GENERIC_CATEGORY;

/** The USB HID generic 'hid' function softstate */
typedef struct {
	usb_hid_dev_t *hid_dev;
} usb_hid_gen_fun_t;

extern errno_t usb_generic_hid_init(usb_hid_dev_t *, void **);
extern void usb_generic_hid_deinit(usb_hid_dev_t *, void *);
extern bool usb_generic_hid_polling_callback(usb_hid_dev_t *, void *);

#endif // USB_HID_HIDDDEV_H_

/**
 * @}
 */
