/*
 * Copyright (c) 2011 Lubos Slovak
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
