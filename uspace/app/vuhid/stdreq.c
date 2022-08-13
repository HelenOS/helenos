/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbvirthid
 * @{
 */
/**
 * @file
 *
 */
#include <errno.h>
#include <usb/debug.h>
#include <usb/descriptor.h>
#include "stdreq.h"
#include "virthid.h"

#define VUHID_DATA(vuhid, device) \
	vuhid_data_t *vuhid = device->device_data

errno_t req_get_descriptor(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	VUHID_DATA(vuhid, device);

	if (setup_packet->value_high == USB_DESCTYPE_HID_REPORT) {
		vuhid_interface_t *iface =
		    vuhid->interface_mapping[setup_packet->index];
		if (iface == NULL) {
			return EFORWARD;
		}
		if (iface->report_descriptor != NULL) {
			usbvirt_control_reply_helper(setup_packet,
			    data, act_size,
			    iface->report_descriptor,
			    iface->report_descriptor_size);
			return EOK;
		} else {
			return ENOENT;
		}
	}

	/* Let the framework handle all the rest. */
	return EFORWARD;
}

errno_t req_set_protocol(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	VUHID_DATA(vuhid, device);

	size_t iface_index = setup_packet->index;
	int protocol = setup_packet->value;

	// FIXME - check ranges
	vuhid_interface_t *iface = vuhid->interface_mapping[iface_index];
	if (iface == NULL) {
		return ENOENT;
	}
	iface->set_protocol = protocol;

	return EOK;
}

errno_t req_set_report(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size)
{
	VUHID_DATA(vuhid, device);

	size_t iface_index = setup_packet->index;

	// FIXME - check ranges
	vuhid_interface_t *iface = vuhid->interface_mapping[iface_index];
	if (iface == NULL) {
		return ENOENT;
	}

	size_t data_length = setup_packet->length;
	if (iface->on_data_out == NULL) {
		return ENOTSUP;
	}

	/* SET_REPORT is translated to data out */
	errno_t rc = iface->on_data_out(iface, data, data_length);

	return rc;
}

/** @}
 */
