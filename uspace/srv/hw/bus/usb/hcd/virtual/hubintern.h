/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief
 */
#ifndef VHCD_HUBINTERN_H_
#define VHCD_HUBINTERN_H_

#include "hub.h"

#define HUB_STATUS_CHANGE_PIPE 1
#define HUB_CONFIGURATION_ID 1

/** Hub descriptor.
 */
typedef struct {
	/** Size of this descriptor in bytes. */
	uint8_t length;
	/** Descriptor type (USB_DESCTYPE_HUB). */
	uint8_t type;
	/** Number of downstream ports. */
	uint8_t port_count;
	/** Hub characteristics. */
	uint16_t characteristics;
	/** Time from power-on to stabilized current.
	 * Expressed in 2ms unit.
	 */
	uint8_t power_on_warm_up;
	/** Maximum current (in mA). */
	uint8_t max_current;
	/** Whether device at given port is removable. */
	uint8_t removable_device[BITS2BYTES(HUB_PORT_COUNT+1)];
	/** Port power control.
	 * This is USB1.0 compatibility field, all bits must be 1.
	 */
	uint8_t port_power[BITS2BYTES(HUB_PORT_COUNT+1)];
} __attribute__ ((packed)) hub_descriptor_t;

typedef enum {
	HUB_PORT_STATE_NOT_CONFIGURED,
	HUB_PORT_STATE_POWERED_OFF,
	HUB_PORT_STATE_DISCONNECTED,
	HUB_PORT_STATE_DISABLED,
	HUB_PORT_STATE_RESETTING,
	HUB_PORT_STATE_ENABLED,
	HUB_PORT_STATE_SUSPENDED,
	HUB_PORT_STATE_RESUMING,
	/* HUB_PORT_STATE_, */
} hub_port_state_t;

typedef enum {
	HUB_STATUS_C_PORT_CONNECTION = (1 << 0),
	HUB_STATUS_C_PORT_ENABLE = (1 << 1),
	HUB_STATUS_C_PORT_SUSPEND = (1 << 2),
	HUB_STATUS_C_PORT_OVER_CURRENT = (1 << 3),
	HUB_STATUS_C_PORT_RESET = (1 << 4),
	/* HUB_STATUS_C_ = (1 << ), */
} hub_status_change_t;

typedef struct {
	virtdev_connection_t *device;
	hub_port_state_t state;
	uint16_t status_change;
} hub_port_t;

typedef struct {
	hub_port_t ports[HUB_PORT_COUNT];
} hub_device_t;

extern hub_device_t hub_dev;

extern hub_descriptor_t hub_descriptor;

extern usbvirt_device_ops_t hub_ops;

void clear_port_status_change(hub_port_t *, uint16_t);
void set_port_status_change(hub_port_t *, uint16_t);


#endif
/**
 * @}
 */
