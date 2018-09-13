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

/** @addtogroup drvusbhid
 * @{
 */
/** @file
 * HID class-specific requests.
 */

#include <stdint.h>
#include <errno.h>
#include <str_error.h>

#include <usb/hid/hid.h>
#include <usb/debug.h>
#include <usb/dev/request.h>
#include <usb/dev/pipes.h>

#include <usb/hid/request.h>

/**
 * Send Set Report request to the HID device.
 *
 * @param hid_dev HID device to send the request to.
 * @param type Type of the report.
 * @param buffer Report data.
 * @param buf_size Report data size (in bytes).
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from function usb_control_request_set().
 */
errno_t usbhid_req_set_report(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	uint16_t value = 0;
	value |= (type << 8);

	usb_log_debug("Sending Set Report request to the device.");

	rc = usb_control_request_set(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_SET_REPORT, value, iface_no, buffer, buf_size);

	if (rc != EOK) {
		usb_log_error("Error sending Set Report request to the "
		    "device: %s.\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/**
 * Send Set Protocol request to the HID device.
 *
 * @param hid_dev HID device to send the request to.
 * @param protocol Protocol to set.
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from function usb_control_request_set().
 */
errno_t usbhid_req_set_protocol(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_protocol_t protocol)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	usb_log_debug("Sending Set Protocol request to the device ("
	    "protocol: %d, iface: %d).\n", protocol, iface_no);

	rc = usb_control_request_set(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_SET_PROTOCOL, protocol, iface_no, NULL, 0);

	if (rc != EOK) {
		usb_log_warning("Error sending Set Protocol request to the "
		    "device: %s.\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/**
 * Send Set Idle request to the HID device.
 *
 * @param hid_dev HID device to send the request to.
 * @param duration Duration value (is multiplicated by 4 by the device to
 *                 get real duration in miliseconds).
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from function usb_control_request_set().
 */
errno_t usbhid_req_set_idle(usb_pipe_t *ctrl_pipe, int iface_no, uint8_t duration)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	usb_log_debug("Sending Set Idle request to the device ("
	    "duration: %u, iface: %d).\n", duration, iface_no);

	uint16_t value = duration << 8;

	rc = usb_control_request_set(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_SET_IDLE, value, iface_no, NULL, 0);

	if (rc != EOK) {
		usb_log_warning("Device did not accept Set Idle request: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/**
 * Send Get Report request to the HID device.
 *
 * @param[in] hid_dev HID device to send the request to.
 * @param[in] type Type of the report.
 * @param[in][out] buffer Buffer for the report data.
 * @param[in] buf_size Size of the buffer (in bytes).
 * @param[out] actual_size Actual size of report received from the device
 *                         (in bytes).
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from function usb_control_request_set().
 */
errno_t usbhid_req_get_report(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size,
    size_t *actual_size)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	uint16_t value = 0;
	value |= (type << 8);

	usb_log_debug("Sending Get Report request to the device.");

	rc = usb_control_request_get(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_GET_REPORT, value, iface_no, buffer, buf_size,
	    actual_size);

	if (rc != EOK) {
		usb_log_warning("Error sending Get Report request to the device: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/**
 * Send Get Protocol request to the HID device.
 *
 * @param[in] hid_dev HID device to send the request to.
 * @param[out] protocol Current protocol of the device.
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from function usb_control_request_set().
 */
errno_t usbhid_req_get_protocol(usb_pipe_t *ctrl_pipe, int iface_no,
    usb_hid_protocol_t *protocol)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	usb_log_debug("Sending Get Protocol request to the device ("
	    "iface: %d).\n", iface_no);

	uint8_t buffer[1];
	size_t actual_size = 0;

	rc = usb_control_request_get(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_GET_PROTOCOL, 0, iface_no, buffer, 1, &actual_size);

	if (rc != EOK) {
		usb_log_warning("Error sending Get Protocol request to the "
		    "device: %s.\n", str_error(rc));
		return rc;
	}

	if (actual_size != 1) {
		usb_log_warning("Wrong data size: %zu, expected: 1.",
		    actual_size);
		return ELIMIT;
	}

	*protocol = buffer[0];

	return EOK;
}

/**
 * Send Get Idle request to the HID device.
 *
 * @param[in] hid_dev HID device to send the request to.
 * @param[out] duration Duration value (multiplicate by 4 to get real duration
 *                      in miliseconds).
 *
 * @retval EOK if successful.
 * @retval EINVAL if no HID device is given.
 * @return Other value inherited from one of functions
 *         usb_pipe_start_session(), usb_pipe_end_session(),
 *         usb_control_request_set().
 */
errno_t usbhid_req_get_idle(usb_pipe_t *ctrl_pipe, int iface_no,
    uint8_t *duration)
{
	if (ctrl_pipe == NULL) {
		usb_log_warning("usbhid_req_set_report(): no pipe given.");
		return EINVAL;
	}

	if (iface_no < 0) {
		usb_log_warning("usbhid_req_set_report(): no interface given."
		    "\n");
		return EINVAL;
	}

	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */

	errno_t rc;

	usb_log_debug("Sending Get Idle request to the device ("
	    "iface: %d).\n", iface_no);

	uint16_t value = 0;
	uint8_t buffer[1];
	size_t actual_size = 0;

	rc = usb_control_request_get(ctrl_pipe,
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE,
	    USB_HIDREQ_GET_IDLE, value, iface_no, buffer, 1,
	    &actual_size);

	if (rc != EOK) {
		usb_log_warning("Error sending Get Idle request to the device: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (actual_size != 1) {
		usb_log_warning("Wrong data size: %zu, expected: 1.",
		    actual_size);
		return ELIMIT;
	}

	*duration = buffer[0];

	return EOK;
}

/**
 * @}
 */
