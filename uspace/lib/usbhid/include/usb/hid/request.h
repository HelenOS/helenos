/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * HID class-specific requests.
 */

#ifndef USB_KBD_HIDREQ_H_
#define USB_KBD_HIDREQ_H_

#include <stdint.h>

#include <usb/hid/hid.h>
#include <usb/dev/pipes.h>

errno_t usbhid_req_set_report(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size);

errno_t usbhid_req_set_protocol(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_protocol_t protocol);

errno_t usbhid_req_set_idle(usb_pipe_t *ctrl_pipe, int iface_no, uint8_t duration);

errno_t usbhid_req_get_report(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size,
    size_t *actual_size);

errno_t usbhid_req_get_protocol(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_protocol_t *protocol);

errno_t usbhid_req_get_idle(usb_pipe_t *ctrl_pipe, int iface_no, uint8_t *duration);

#endif /* USB_KBD_HIDREQ_H_ */

/**
 * @}
 */
