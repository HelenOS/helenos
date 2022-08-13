/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB Keyboard multimedia keys subdriver.
 */

#ifndef USB_HID_MULTIMEDIA_H_
#define USB_HID_MULTIMEDIA_H_

#include <usb/dev/driver.h>
#include "../usbhid.h"

extern errno_t usb_multimedia_init(usb_hid_dev_t *, void **);
extern void usb_multimedia_deinit(usb_hid_dev_t *, void *);
extern bool usb_multimedia_polling_callback(usb_hid_dev_t *, void *);

#endif // USB_HID_MULTIMEDIA_H_

/**
 * @}
 */
