/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB Mouse driver API.
 */

#ifndef USB_HID_MOUSEDEV_H_
#define USB_HID_MOUSEDEV_H_

#include <usb/dev/driver.h>
#include <async.h>
#include "../usbhid.h"

/** Container for USB mouse device. */
typedef struct {
	/** IPC session to consumer. */
	async_sess_t *mouse_sess;

	/** Mouse buttons statuses. */
	int32_t *buttons;
	size_t buttons_count;

	/** DDF mouse function */
	ddf_fun_t *mouse_fun;
} usb_mouse_t;

extern const usb_endpoint_description_t usb_hid_mouse_poll_endpoint_description;

extern const char *HID_MOUSE_FUN_NAME;
extern const char *HID_MOUSE_CATEGORY;

extern errno_t usb_mouse_init(usb_hid_dev_t *, void **);
extern bool usb_mouse_polling_callback(usb_hid_dev_t *, void *);
extern void usb_mouse_deinit(usb_hid_dev_t *, void *);
extern errno_t usb_mouse_set_boot_protocol(usb_hid_dev_t *);

#endif // USB_HID_MOUSEDEV_H_

/**
 * @}
 */
