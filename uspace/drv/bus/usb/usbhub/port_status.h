/*
 * Copyright (c) 2010 Matus Dekanek
 * Copyright (c) 2011 Jan Vesely
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
#include <usb/dev/request.h>

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
 *
 * For more information refer to table 11.16.2.5 in
 * "Universal Serial Bus Specification Revision 1.1"
 *
 */
typedef uint32_t usb_hub_status_t;
// TODO Mind the endiannes, changes are in the first byte of the second word
// status is int he first byte of the first word
#define USB_HUB_STATUS_OVER_CURRENT \
    (1 << (USB_HUB_FEATURE_HUB_OVER_CURRENT))
#define USB_HUB_STATUS_LOCAL_POWER \
    (1 << (USB_HUB_FEATURE_HUB_LOCAL_POWER))

#define USB_HUB_STATUS_C_OVER_CURRENT \
    (1 << (16 + USB_HUB_FEATURE_C_HUB_OVER_CURRENT))
#define USB_HUB_STATUS_C_LOCAL_POWER \
    (1 << (16 + USB_HUB_FEATURE_C_HUB_LOCAL_POWER))

/**
 * set the device request to be a port feature enable request
 * @param request
 * @param port
 * @param feature_selector
 */
static inline void usb_hub_set_enable_port_feature_request(
    usb_device_request_setup_packet_t *request, uint16_t port,
    uint16_t feature_selector) {
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
    usb_device_request_setup_packet_t *request, uint16_t port,
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
    usb_device_request_setup_packet_t *request, uint16_t port
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
	usb_device_request_setup_packet_t *result =
	    malloc(sizeof (usb_device_request_setup_packet_t));
	usb_hub_set_enable_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_disable_port_request(
    usb_device_request_setup_packet_t *request, uint16_t port
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
	usb_device_request_setup_packet_t *result =
	    malloc(sizeof (usb_device_request_setup_packet_t));
	usb_hub_set_disable_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_reset_port_request(
    usb_device_request_setup_packet_t *request, uint16_t port
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
	usb_device_request_setup_packet_t *result =
	    malloc(sizeof (usb_device_request_setup_packet_t));
	usb_hub_set_reset_port_request(result, port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_power_port_request(
    usb_device_request_setup_packet_t *request, uint16_t port
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
    usb_device_request_setup_packet_t *request, uint16_t port
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
static inline bool usb_port_is_status(usb_port_status_t status, int idx) {
	return (status & (1 << idx)) != 0;
}

/**
 * set i`th bit of port status
 * 
 * @param status
 * @param idx
 * @param value
 */
static inline void usb_port_status_set_bit(
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
static inline bool usb_hub_is_status(usb_hub_status_t status, int idx) {
	return (status & (1 << idx)) != 0;
}

/**
 * set i`th bit of hub status
 * 
 * @param status
 * @param idx
 * @param value
 */
static inline void usb_hub_status_set_bit(
    usb_hub_status_t *status, int idx, bool value) {
	(*status) = value ?
	    ((*status) | (1 << (idx))) :
	    ((*status)&(~(1 << (idx))));
}

/**
 * low speed device on the port indicator
 * 
 * @param status
 * @return true if low speed device is attached
 */
static inline bool usb_port_low_speed(usb_port_status_t status) {
	return usb_port_is_status(status, 9);
}

/**
 * set low speed device connected bit in port status
 * 
 * @param status
 * @param low_speed value of the bit
 */
static inline void usb_port_set_low_speed(usb_port_status_t *status, bool low_speed) {
	usb_port_status_set_bit(status, 9, low_speed);
}

//high speed device attached

/**
 * high speed device on the port indicator
 *
 * @param status
 * @return true if high speed device is on port
 */
static inline bool usb_port_high_speed(usb_port_status_t status) {
	return usb_port_is_status(status, 10);
}

/**
 * set high speed device bit in port status
 *
 * @param status
 * @param high_speed value of the bit
 */
static inline void usb_port_set_high_speed(usb_port_status_t *status, bool high_speed) {
	usb_port_status_set_bit(status, 10, high_speed);
}

/**
 * speed getter for port status
 *
 * @param status
 * @return speed of usb device (for more see usb specification)
 */
static inline usb_speed_t usb_port_speed(usb_port_status_t status) {
	if (usb_port_low_speed(status))
		return USB_SPEED_LOW;
	if (usb_port_high_speed(status))
		return USB_SPEED_HIGH;
	return USB_SPEED_FULL;
}



#endif	/* HUB_PORT_STATUS_H */
/**
 * @}
 */
