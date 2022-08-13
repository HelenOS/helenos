/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbvirt
 * @{
 */
/** @file
 * Private definitions.
 */
#ifndef USBVIRT_PRIVATE_H_
#define USBVIRT_PRIVATE_H_

#include <usbvirt/device.h>

errno_t process_control_transfer(usbvirt_device_t *,
    const usbvirt_control_request_handler_t *,
    const usb_device_request_setup_packet_t *,
    uint8_t *, size_t *);

extern usbvirt_control_request_handler_t library_handlers[];

#endif
/**
 * @}
 */
