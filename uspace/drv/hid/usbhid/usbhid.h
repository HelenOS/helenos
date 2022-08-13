/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 * SPDX-FileCopyrightText: 2018 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * USB HID driver API.
 */

#ifndef USB_HID_USBHID_H_
#define USB_HID_USBHID_H_

#include <stdint.h>

#include <usb/hid/hidparser.h>
#include <ddf/driver.h>
#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>
#include <usb/dev/poll.h>
#include <usb/hid/hid.h>
#include <stdbool.h>

typedef struct usb_hid_dev usb_hid_dev_t;
typedef struct usb_hid_subdriver usb_hid_subdriver_t;

/** Subdriver initialization callback.
 *
 * @param dev Backing USB HID device.
 * @param data Custom subdriver data (pointer where to store them).
 * @return Error code.
 */
typedef errno_t (*usb_hid_driver_init_t)(usb_hid_dev_t *dev, void **data);

/** Subdriver deinitialization callback.
 *
 * @param dev Backing USB HID device.
 * @param data Custom subdriver data.
 */
typedef void (*usb_hid_driver_deinit_t)(usb_hid_dev_t *dev, void *data);

/** Subdriver callback on data from device.
 *
 * @param dev Backing USB HID device.
 * @param data Custom subdriver data.
 * @return Whether to continue polling (typically true always).
 */
typedef bool (*usb_hid_driver_poll_t)(usb_hid_dev_t *dev, void *data);

/** Subdriver callback after communication with the device ceased.
 *
 * @param dev Backing USB HID device.
 * @param data Custom subdriver data.
 * @param ended_due_to_errors Whether communication ended due to errors in
 *	communication (true) or deliberately by driver (false).
 */
typedef void (*usb_hid_driver_poll_ended_t)(usb_hid_dev_t *dev, void *data,
    bool ended_due_to_errors);

struct usb_hid_subdriver {
	/** Function to be called when initializing HID device. */
	usb_hid_driver_init_t init;
	/** Function to be called when destroying the HID device structure. */
	usb_hid_driver_deinit_t deinit;
	/** Function to be called when data arrives from the device. */
	usb_hid_driver_poll_t poll;
	/** Function to be called when polling ends. */
	usb_hid_driver_poll_ended_t poll_end;
	/** Arbitrary data needed by the subdriver. */
	void *data;
};

/**
 * Structure for holding general HID device data.
 */
struct usb_hid_dev {
	/** Structure holding generic USB device information. */
	usb_device_t *usb_dev;

	/** Endpont mapping of the polling pipe. */
	usb_endpoint_mapping_t *poll_pipe_mapping;

	/** Device polling structure. */
	usb_polling_t polling;

	/** Subdrivers. */
	usb_hid_subdriver_t *subdrivers;

	/** Number of subdrivers. */
	unsigned subdriver_count;

	/** Report descriptor. */
	uint8_t *report_desc;

	/** Report descriptor size. */
	size_t report_desc_size;

	/** HID Report parser. */
	usb_hid_report_t report;

	uint8_t report_id;

	uint8_t *input_report;

	size_t input_report_size;
	size_t max_input_report_size;

	int report_nr;
	volatile bool running;
};

extern const usb_endpoint_description_t *usb_hid_endpoints[];

errno_t usb_hid_init(usb_hid_dev_t *hid_dev, usb_device_t *dev);

void usb_hid_deinit(usb_hid_dev_t *hid_dev);

void usb_hid_new_report(usb_hid_dev_t *hid_dev);

int usb_hid_report_number(const usb_hid_dev_t *hid_dev);

#endif /* USB_HID_USBHID_H_ */

/**
 * @}
 */
