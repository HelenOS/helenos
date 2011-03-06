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

#include <usb/classes/hid.h>
#include <usb/debug.h>
#include <usb/request.h>

#include "hidreq.h"
#include "hiddev.h"

/*----------------------------------------------------------------------------*/

int usbhid_req_set_report(usbhid_dev_t *hid_dev,
    usb_hid_report_type_t type, uint8_t *buffer, size_t buf_size)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_report(): no HID device structure"
		    " given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	uint16_t value = 0;
	value |= (type << 8);

	usb_log_debug("Sending Set_Report request to the device.\n");
	
	rc = usb_control_request_set(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_SET_REPORT, value, hid_dev->iface, buffer, buf_size);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

int usbhid_req_set_protocol(usbhid_dev_t *hid_dev, usb_hid_protocol_t protocol)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_protocol(): no HID device "
		    "structure given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}

	usb_log_debug("Sending Set_Protocol request to the device ("
	    "protocol: %d, iface: %d).\n", protocol, hid_dev->iface);
	
	rc = usb_control_request_set(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_SET_PROTOCOL, protocol, hid_dev->iface, NULL, 0);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

int usbhid_req_set_idle(usbhid_dev_t *hid_dev, uint8_t duration)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_idle(): no HID device "
		    "structure given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}

	usb_log_debug("Sending Set_Idle request to the device ("
	    "duration: %u, iface: %d).\n", duration, hid_dev->iface);
	
	uint16_t value = duration << 8;
	
	rc = usb_control_request_set(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_SET_IDLE, value, hid_dev->iface, NULL, 0);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

int usbhid_req_get_report(usbhid_dev_t *hid_dev, usb_hid_report_type_t type, 
    uint8_t *buffer, size_t buf_size, size_t *actual_size)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_report(): no HID device structure"
		    " given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}

	uint16_t value = 0;
	value |= (type << 8);
	
	usb_log_debug("Sending Get_Report request to the device.\n");
	
	rc = usb_control_request_get(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_GET_REPORT, value, hid_dev->iface, buffer, buf_size,
	    actual_size);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	return EOK;
}

int usbhid_req_get_protocol(usbhid_dev_t *hid_dev, usb_hid_protocol_t *protocol)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_protocol(): no HID device "
		    "structure given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}

	usb_log_debug("Sending Get_Protocol request to the device ("
	    "protocol: %d, iface: %d).\n", protocol, hid_dev->iface);
	
	uint8_t buffer[1];
	size_t actual_size = 0;
	
	rc = usb_control_request_get(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_GET_PROTOCOL, 0, hid_dev->iface, buffer, 1, &actual_size);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	if (actual_size != 1) {
		usb_log_warning("Wrong data size: %zu, expected: 1.\n",
			actual_size);
		return ELIMIT;
	}
	
	*protocol = buffer[0];
	
	return EOK;
}

int usbhid_req_get_idle(usbhid_dev_t *hid_dev, uint8_t *duration)
{
	if (hid_dev == NULL) {
		usb_log_error("usbhid_req_set_idle(): no HID device "
		    "structure given.\n");
		return EINVAL;
	}
	
	/*
	 * No need for checking other parameters, as they are checked in
	 * the called function (usb_control_request_set()).
	 */
	
	int rc, sess_rc;
	
	sess_rc = usb_endpoint_pipe_start_session(&hid_dev->ctrl_pipe);
	if (sess_rc != EOK) {
		usb_log_warning("Failed to start a session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}

	usb_log_debug("Sending Get_Idle request to the device ("
	    "duration: %u, iface: %d).\n", duration, hid_dev->iface);
	
	uint16_t value = 0;
	uint8_t buffer[1];
	size_t actual_size = 0;
	
	rc = usb_control_request_get(&hid_dev->ctrl_pipe, 
	    USB_REQUEST_TYPE_CLASS, USB_REQUEST_RECIPIENT_INTERFACE, 
	    USB_HIDREQ_GET_IDLE, value, hid_dev->iface, buffer, 1, 
	    &actual_size);

	sess_rc = usb_endpoint_pipe_end_session(&hid_dev->ctrl_pipe);

	if (rc != EOK) {
		usb_log_warning("Error sending output report to the keyboard: "
		    "%s.\n", str_error(rc));
		return rc;
	}

	if (sess_rc != EOK) {
		usb_log_warning("Error closing session: %s.\n",
		    str_error(sess_rc));
		return sess_rc;
	}
	
	if (actual_size != 1) {
		usb_log_warning("Wrong data size: %zu, expected: 1.\n",
			actual_size);
		return ELIMIT;
	}
	
	*duration = buffer[0];
	
	return EOK;
}

/*----------------------------------------------------------------------------*/

/**
 * @}
 */
