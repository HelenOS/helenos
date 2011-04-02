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
 * set values in request to be it a port status request
 * @param request
 * @param port
 */
static inline void usb_hub_set_port_status_request(
usb_device_request_setup_packet_t * request, uint16_t port
){
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_GET_PORT_STATUS;
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
usb_hub_create_port_status_request(uint16_t port){
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_port_status_request(result,port);
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
){
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
){
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
){
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
usb_hub_create_enable_port_request(uint16_t port){
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_enable_port_request(result,port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_disable_port_request(
usb_device_request_setup_packet_t * request, uint16_t port
){
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
usb_hub_create_disable_port_request(uint16_t port){
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_disable_port_request(result,port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_reset_port_request(
usb_device_request_setup_packet_t * request, uint16_t port
){
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
usb_hub_create_reset_port_request(uint16_t port){
	usb_device_request_setup_packet_t * result =
		usb_new(usb_device_request_setup_packet_t);
	usb_hub_set_reset_port_request(result,port);
	return result;
}

/**
 * set the device request to be a port disable request
 * @param request
 * @param port
 */
static inline void usb_hub_set_power_port_request(
usb_device_request_setup_packet_t * request, uint16_t port
){
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
){
	request->index = port;
	request->request_type = USB_HUB_REQ_TYPE_SET_PORT_FEATURE;
	request->request = USB_HUB_REQUEST_CLEAR_FEATURE;
	request->value = USB_HUB_FEATURE_PORT_POWER;
	request->length = 0;
}


/** get i`th bit of port status */
static inline bool usb_port_get_bit(usb_port_status_t * status, int idx)
{
	return (((*status)>>(idx))%2);
}

/** set i`th bit of port status */
static inline void usb_port_set_bit(
	usb_port_status_t * status, int idx, bool value)
{
	(*status) = value?
		               ((*status)|(1<<(idx))):
		               ((*status)&(~(1<<(idx))));
}

//device connnected on port
static inline bool usb_port_dev_connected(usb_port_status_t * status){
	return usb_port_get_bit(status,0);
}

static inline void usb_port_set_dev_connected(usb_port_status_t * status,bool connected){
	usb_port_set_bit(status,0,connected);
}

//port enabled
static inline bool usb_port_enabled(usb_port_status_t * status){
	return usb_port_get_bit(status,1);
}

static inline void usb_port_set_enabled(usb_port_status_t * status,bool enabled){
	usb_port_set_bit(status,1,enabled);
}

//port suspended
static inline bool usb_port_suspended(usb_port_status_t * status){
	return usb_port_get_bit(status,2);
}

static inline void usb_port_set_suspended(usb_port_status_t * status,bool suspended){
	usb_port_set_bit(status,2,suspended);
}

//over currect
static inline bool usb_port_over_current(usb_port_status_t * status){
	return usb_port_get_bit(status,3);
}

static inline void usb_port_set_over_current(usb_port_status_t * status,bool value){
	usb_port_set_bit(status,3,value);
}

//port reset
static inline bool usb_port_reset(usb_port_status_t * status){
	return usb_port_get_bit(status,4);
}

static inline void usb_port_set_reset(usb_port_status_t * status,bool value){
	usb_port_set_bit(status,4,value);
}

//powered
static inline bool usb_port_powered(usb_port_status_t * status){
	return usb_port_get_bit(status,8);
}

static inline void usb_port_set_powered(usb_port_status_t * status,bool powered){
	usb_port_set_bit(status,8,powered);
}

//low speed device attached
static inline bool usb_port_low_speed(usb_port_status_t * status){
	return usb_port_get_bit(status,9);
}

static inline void usb_port_set_low_speed(usb_port_status_t * status,bool low_speed){
	usb_port_set_bit(status,9,low_speed);
}

//low speed device attached
static inline bool usb_port_high_speed(usb_port_status_t * status){
	return usb_port_get_bit(status,10);
}

static inline void usb_port_set_high_speed(usb_port_status_t * status,bool high_speed){
	usb_port_set_bit(status,10,high_speed);
}

static inline usb_speed_t usb_port_speed(usb_port_status_t * status){
	if(usb_port_low_speed(status))
		return USB_SPEED_LOW;
	if(usb_port_high_speed(status))
		return USB_SPEED_HIGH;
	return USB_SPEED_FULL;
}


//connect change
static inline bool usb_port_connect_change(usb_port_status_t * status){
	return usb_port_get_bit(status,16);
}

static inline void usb_port_set_connect_change(usb_port_status_t * status,bool change){
	usb_port_set_bit(status,16,change);
}

//port enable change
static inline bool usb_port_enabled_change(usb_port_status_t * status){
	return usb_port_get_bit(status,17);
}

static inline void usb_port_set_enabled_change(usb_port_status_t * status,bool change){
	usb_port_set_bit(status,17,change);
}

//suspend change
static inline bool usb_port_suspend_change(usb_port_status_t * status){
	return usb_port_get_bit(status,18);
}

static inline void usb_port_set_suspend_change(usb_port_status_t * status,bool change){
	usb_port_set_bit(status,18,change);
}

//over current change
static inline bool usb_port_overcurrent_change(usb_port_status_t * status){
	return usb_port_get_bit(status,19);
}

static inline void usb_port_set_overcurrent_change(usb_port_status_t * status,bool change){
	usb_port_set_bit(status,19,change);
}

//reset change
static inline bool usb_port_reset_completed(usb_port_status_t * status){
	return usb_port_get_bit(status,20);
}

static inline void usb_port_set_reset_completed(usb_port_status_t * status,bool completed){
	usb_port_set_bit(status,20,completed);
}



#endif	/* HUB_PORT_STATUS_H */

/**
 * @}
 */
