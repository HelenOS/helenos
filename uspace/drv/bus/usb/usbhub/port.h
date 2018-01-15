/*
 * Copyright (c) 2011 Vojtech Horky
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Ondra Hlavaty
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
/** @file
 * Hub port state machine.
 */

#ifndef DRV_USBHUB_PORT_H
#define DRV_USBHUB_PORT_H

#include <usb/dev/driver.h>
#include <usb/classes/hub.h>

typedef struct usb_hub_dev usb_hub_dev_t;

typedef enum {
	PORT_DISABLED,	/* No device connected. */
	PORT_CONNECTED,	/* A device connected, not yet initialized. */
	PORT_IN_RESET,	/* An initial port reset in progress. */
	PORT_ENABLED,	/* Port reset complete, port enabled. Device announced to the HC. */
	PORT_ERROR,	/* An error occured. There is still a fibril that needs to know it. */
} port_state_t;

/** Information about single port on a hub. */
typedef struct {
	/* Parenting hub */
	usb_hub_dev_t *hub;
	/** Guarding all fields */
	fibril_mutex_t guard;
	/** Current state of the port */
	port_state_t state;
	/** A speed of the device connected (if any). Valid unless state == PORT_DISABLED. */
	usb_speed_t speed;
	/** Port number as reported in descriptors. */
	unsigned int port_number;
	/** CV for waiting to port reset completion. */
	fibril_condvar_t state_cv;
} usb_hub_port_t;

void usb_hub_port_init(usb_hub_port_t *, usb_hub_dev_t *, unsigned int);
void usb_hub_port_fini(usb_hub_port_t *);

void usb_hub_port_process_interrupt(usb_hub_port_t *port, usb_hub_dev_t *hub);

#endif

/**
 * @}
 */
