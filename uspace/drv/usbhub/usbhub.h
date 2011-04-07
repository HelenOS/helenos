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

/** @addtogroup drvusbhub
 * @{
 */
/** @file
 * @brief Hub driver.
 */
#ifndef DRV_USBHUB_USBHUB_H
#define DRV_USBHUB_USBHUB_H

#include <ipc/devman.h>
#include <usb/usb.h>
#include <ddf/driver.h>

#define NAME "usbhub"

#include <usb/hub.h>

#include <usb/pipes.h>
#include <usb/devdrv.h>

#include "ports.h"



/** Information about attached hub. */
typedef struct {
	/** Number of ports. */
	size_t port_count;

	/** Ports. */
	usb_hub_port_t *ports;
	
	/** connection to hcd */
	usb_hc_connection_t connection;

	/** default address is used indicator
	 *
	 * If default address is requested by this device, it cannot
	 * be requested by the same hub again, otherwise a deadlock will occur.
	 */
	bool is_default_address_used;

	/** convenience pointer to status change pipe
	 *
	 * Status change pipe is initialized in usb_device structure. This is
	 * pointer into this structure, so that it does not have to be
	 * searched again and again for the 'right pipe'.
	 */
	usb_pipe_t * status_change_pipe;

	/** convenience pointer to control pipe
	 *
	 * Control pipe is initialized in usb_device structure. This is
	 * pointer into this structure, so that it does not have to be
	 * searched again and again for the 'right pipe'.
	 */
	usb_pipe_t * control_pipe;

	/** generic usb device data*/
	usb_device_t * usb_device;
} usb_hub_info_t;

/**
 * function running the hub-controlling loop.
 * @param hub_info_param hub info pointer
 */
int usb_hub_control_loop(void * hub_info_param);

/**
 * Check changes on specified hub
 * @param hub_info_param pointer to usb_hub_info_t structure
 * @return error code if there is problem when initializing communication with
 * hub, EOK otherwise
 */
int usb_hub_check_hub_changes(usb_hub_info_t * hub_info_param);

void usb_hub_removed_device(usb_hub_info_t *, uint16_t);
void usb_hub_over_current(usb_hub_info_t *, uint16_t);

int usb_hub_add_device(usb_device_t * usb_dev);

#endif
/**
 * @}
 */
