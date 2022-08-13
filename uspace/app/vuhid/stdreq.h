/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbvirthid
 * @{
 */
/** @file
 * Device request handlers.
 */
#ifndef VUHID_STDREQ_H_
#define VUHID_STDREQ_H_

#include <usbvirt/device.h>

errno_t req_get_descriptor(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size);

errno_t req_set_protocol(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size);

errno_t req_set_report(usbvirt_device_t *device,
    const usb_device_request_setup_packet_t *setup_packet,
    uint8_t *data, size_t *act_size);

#endif
/**
 * @}
 */
