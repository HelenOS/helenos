/*
 * Copyright (c) 2010 Matus Dekanek
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
/** @addtogroup drvusbhub
 * @{
 */

#ifndef HUB_PORT_STATUS_H
#define	HUB_PORT_STATUS_H

#include <bool.h>
#include <sys/types.h>
#include <usb/request.h>
#include "usbhub_private.h"

/**
 * structure holding port status and changes flags.
 * should not be accessed directly, use supplied getter/setter methods.
 *
 * For more information refer to table 11-15 in
 * "Universal Serial Bus Specification Revision 1.1"
 *
 */
typedef uint32_t usb_port_status_t;

/**
 * structure holding hub status and changes flags.
 * should not be accessed directly, use supplied getter/setter methods.
 *
 * For more information refer to table 11.16.2.5 in
 * "Universal Serial Bus Specification Revision 1.1"
 *
 */
typedef uint32_t usb_hub_status_t;

/**
 * set values in request to be it a port status request
 * @param request
 * @param port
 */
static inline void usb_hub_set_port_status_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_GET_PORT_STATUS;
	request->request = USB_HUB_REQUEST_GET_STATUS;
	request->value = 0;
	request->length = 4;
}

/**
 * set values in request to be it a port status request
 * @param request
 * @param port
 */
static inline void usb_hub_set_hub_status_request(
	usb_device_request_setup_packet_t * request
	) {
	request->index = 0;
	request->request_type = USB_HUB_REQ_TYPE_GET_HUB_STATUS;
	request->request = USB_HUB_REQUEST_GET_STATUS;
	request->value = 0;
	request->length = 4;
}

/**
 * create request for usb hub port status
 * @param port
 * @return
 */
static inline usb_device_request_setup_packet_t *
usb_hub_create_port_status_request(uint16_t port) {
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_port_status_request(result, port);
	return result;
}

/**
 * set the device request to be a port feature enable request
 * @param request
 * @param port
 * @param feature_selector
 */
static inline void usb_hub_set_enable_port_feature_request(
	usb_device_request_setup_packet_t * request, uint16_t port,
	uint16_t feature_selector
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_SET_FEATURE;
	request->value = feature_selector;
	request->length = 0;
}

/**
 * set the device request to be a port feature clear request
 * @param request
 * @param port
 * @param feature_selector
 */
static inline void usb_hub_set_disable_port_feature_request(
	usb_device_request_setup_packet_t * request, uint16_t port,
	uint16_t feature_selector
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_CLEAR_FEATURE;
	request->value = feature_selector;
	request->length = 0;
}

/**
 * set the device request to be a port enable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_enable_port_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_SET_FEATURE;
	request->value = USB_HUB_FEATURE_C_PORT_ENABLE;
	request->length = 0;
}

/**
 * enable specified port
 * @param port
 * @return
 */
static inline usb_device_request_setup_packet_t *
usb_hub_create_enable_port_request(uint16_t port) {
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_enable_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_disable_port_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_SET_FEATURE;
	request->value = USB_HUB_FEATURE_C_PORT_SUSPEND;
	request->length = 0;
}

/**
 * disable specified port
 * @param port
 * @return
 */
static inline usb_device_request_setup_packet_t *
usb_hub_create_disable_port_request(uint16_t port) {
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_disable_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_reset_port_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_SET_FEATURE;
	request->value = USB_HUB_FEATURE_PORT_RESET;
	request->length = 0;
}

/**
 * disable specified port
 * @param port
 * @return
 */
static inline usb_device_request_setup_packet_t *
usb_hub_create_reset_port_request(uint16_t port) {
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_reset_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_power_port_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_SET_FEATURE;
	request->value = USB_HUB_FEATURE_PORT_POWER;
	request->length = 0;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_unset_power_port_request(
	usb_device_request_setup_packet_t * request, uint16_t port
	) {
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_CLEAR_FEATURE;
	request->value = USB_HUB_FEATURE_PORT_POWER;
	request->length = 0;
}

/**
 * get i`th bit of port status
 * 
 * @param status
 * @param idx
 * @return
 */
static inline bool usb_port_get_bit(usb_port_status_t * status, int idx) {
	return (*status)&(1 << idx);
}

/**
 * set i`th bit of port status
 * 
 * @param status
 * @param idx
 * @param value
 */
static inline void usb_port_set_bit(
	usb_port_status_t * status, int idx, bool value) {
	(*status) = value ?
		((*status) | (1 << (idx))) :
		((*status)&(~(1 << (idx))));
}

/**
 * get i`th bit of hub status
 * 
 * @param status
 * @param idx
 * @return
 */
static inline bool usb_hub_get_bit(usb_hub_status_t * status, int idx) {
	return (*status)&(1 << idx);
}

/**
 * set i`th bit of hub status
 * 
 * @param status
 * @param idx
 * @param value
 */
static inline void usb_hub_set_bit(
	usb_hub_status_t * status, int idx, bool value) {
	(*status) = value ?
		((*status) | (1 << (idx))) :
		((*status)&(~(1 << (idx))));
}

/**
 * connection status geter for port status
 * 
 * @param status
 * @return true if there is something connected
 */
static inline bool usb_port_dev_connected(usb_port_status_t * status) {
	return usb_port_get_bit(status, 0);
}

static inline void usb_port_set_dev_connected(usb_port_status_t * status, bool connected) {
	usb_port_set_bit(status, 0, connected);
}

//port enabled

/**
 * port enabled getter for port status
 * 
 * @param status
 * @return true if the port is enabled
 */
static inline bool usb_port_enabled(usb_port_status_t * status) {
	return usb_port_get_bit(status, 1);
}

static inline void usb_port_set_enabled(usb_port_status_t * status, bool enabled) {
	usb_port_set_bit(status, 1, enabled);
}

//port suspended
/**
 * port suspended getter for port status
 *
 * @param status
 * @return true if port is suspended
 */
static inline bool usb_port_suspended(usb_port_status_t * status) {
	return usb_port_get_bit(status, 2);
}

static inline void usb_port_set_suspended(usb_port_status_t * status, bool suspended) {
	usb_port_set_bit(status, 2, suspended);
}

//over currect
/**
 * over current condition indicator getter for port status
 *
 * @param status
 * @return true if there is opver-current condition on the hub
 */
static inline bool usb_port_over_current(usb_port_status_t * status) {
	return usb_port_get_bit(status, 3);
}

static inline void usb_port_set_over_current(usb_port_status_t * status, bool value) {
	usb_port_set_bit(status, 3, value);
}

//port reset
/**
 * port reset indicator getter for port status
 * 
 * @param status
 * @return true if port is reset
 */
static inline bool usb_port_reset(usb_port_status_t * status) {
	return usb_port_get_bit(status, 4);
}

static inline void usb_port_set_reset(usb_port_status_t * status, bool value) {
	usb_port_set_bit(status, 4, value);
}

//powered
/**
 * power state getter for port status
 *
 * @param status
 * @return true if port is powered
 */
static inline bool usb_port_powered(usb_port_status_t * status) {
	return usb_port_get_bit(status, 8);
}

static inline void usb_port_set_powered(usb_port_status_t * status, bool powered) {
	usb_port_set_bit(status, 8, powered);
}

//low speed device attached
/**
 * low speed device on the port indicator
 * 
 * @param status
 * @return true if low speed device is attached
 */
static inline bool usb_port_low_speed(usb_port_status_t * status) {
	return usb_port_get_bit(status, 9);
}

static inline void usb_port_set_low_speed(usb_port_status_t * status, bool low_speed) {
	usb_port_set_bit(status, 9, low_speed);
}

//high speed device attached
/**
 * high speed device on the port indicator
 *
 * @param status
 * @return true if high speed device is on port
 */
static inline bool usb_port_high_speed(usb_port_status_t * status) {
	return usb_port_get_bit(status, 10);
}

static inline void usb_port_set_high_speed(usb_port_status_t * status, bool high_speed) {
	usb_port_set_bit(status, 10, high_speed);
}

/**
 * speed getter for port status
 *
 * @param status
 * @return speed of usb device (for more see usb specification)
 */
static inline usb_speed_t usb_port_speed(usb_port_status_t * status) {
	if (usb_port_low_speed(status))
		return USB_SPEED_LOW;
	if (usb_port_high_speed(status))
		return USB_SPEED_HIGH;
	return USB_SPEED_FULL;
}


//connect change
/**
 * port connect change indicator
 *
 * @param status
 * @return true if connection has changed
 */
static inline bool usb_port_connect_change(usb_port_status_t * status) {
	return usb_port_get_bit(status, 16);
}

static inline void usb_port_set_connect_change(usb_port_status_t * status, bool change) {
	usb_port_set_bit(status, 16, change);
}

//port enable change
/**
 * port enable change for port status
 *
 * @param status
 * @return true if the port has been enabled/disabled
 */
static inline bool usb_port_enabled_change(usb_port_status_t * status) {
	return usb_port_get_bit(status, 17);
}

static inline void usb_port_set_enabled_change(usb_port_status_t * status, bool change) {
	usb_port_set_bit(status, 17, change);
}

//suspend change
/**
 * port suspend change for port status
 * 
 * @param status
 * @return ture if suspend status has changed
 */
static inline bool usb_port_suspend_change(usb_port_status_t * status) {
	return usb_port_get_bit(status, 18);
}

static inline void usb_port_set_suspend_change(usb_port_status_t * status, bool change) {
	usb_port_set_bit(status, 18, change);
}

//over current change
/**
 * over current change indicator
 * 
 * @param status
 * @return true if over-current condition on port has changed
 */
static inline bool usb_port_overcurrent_change(usb_port_status_t * status) {
	return usb_port_get_bit(status, 19);
}

static inline void usb_port_set_overcurrent_change(usb_port_status_t * status, bool change) {
	usb_port_set_bit(status, 19, change);
}

//reset change
/**
 * port reset change indicator
 * @param status
 * @return true if port has been reset
 */
static inline bool usb_port_reset_completed(usb_port_status_t * status) {
	return usb_port_get_bit(status, 20);
}

static inline void usb_port_set_reset_completed(usb_port_status_t * status, bool completed) {
	usb_port_set_bit(status, 20, completed);
}

//local power status
/**
 * local power lost indicator for hub status
 * 
 * @param status
 * @return true if hub is not powered
 */
static inline bool usb_hub_local_power_lost(usb_hub_status_t * status) {
	return usb_hub_get_bit(status, 0);
}

static inline void usb_hub_set_local_power_lost(usb_port_status_t * status,
	bool power_lost) {
	usb_hub_set_bit(status, 0, power_lost);
}

//over current ocndition
/**
 * hub over-current indicator
 *
 * @param status
 * @return true if over-current condition occurred on hub
 */
static inline bool usb_hub_over_current(usb_hub_status_t * status) {
	return usb_hub_get_bit(status, 1);
}

static inline void usb_hub_set_over_current(usb_port_status_t * status,
	bool over_current) {
	usb_hub_set_bit(status, 1, over_current);
}

//local power change
/**
 * hub power change indicator
 *
 * @param status
 * @return true if local power status has been changed - power has been
 * dropped or re-established
 */
static inline bool usb_hub_local_power_change(usb_hub_status_t * status) {
	return usb_hub_get_bit(status, 16);
}

static inline void usb_hub_set_local_power_change(usb_port_status_t * status,
	bool change) {
	usb_hub_set_bit(status, 16, change);
}

//local power status
/**
 * hub over-current condition change indicator
 *
 * @param status
 * @return true if over-current condition has changed
 */
static inline bool usb_hub_over_current_change(usb_hub_status_t * status) {
	return usb_hub_get_bit(status, 17);
}

static inline void usb_hub_set_over_current_change(usb_port_status_t * status,
	bool change) {
	usb_hub_set_bit(status, 17, change);
}


#endif	/* HUB_PORT_STATUS_H */

/**
 * @}
 */
