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

/* Hub endpoints. */
typedef struct {
        usb_endpoint_pipe_t control;
        usb_endpoint_pipe_t status_change;
} usb_hub_endpoints_t;



/** Information about attached hub. */
typedef struct {
	/** Number of ports. */
	int port_count;
	/** attached device handles, for each port one */
	usb_hc_attached_device_t * attached_devs;
	/** General usb device info. */
	//usb_hcd_attached_device_info_t * usb_device;
	/** General device info*/
	ddf_dev_t * device;
	/** connection to hcd */
	//usb_device_connection_t connection;
	usb_hc_connection_t connection;
	/** */
	usb_device_connection_t device_connection;
	/** hub endpoints */
	usb_hub_endpoints_t endpoints;
} usb_hub_info_t;

/**
 * function running the hub-controlling loop.
 * @param hub_info_param hub info pointer
 */
int usb_hub_control_loop(void * hub_info_param);

/** Callback when new hub device is detected.
 *
 * @param dev New device.
 * @return Error code.
 */
int usb_add_hub_device(ddf_dev_t *dev);

/**
 * check changes on all registered hubs
 */
//void usb_hub_check_hub_changes(void);

/**
 * check changes on specified hub
 * @param hub_info_param pointer to usb_hub_info_t structure
 */
void usb_hub_check_hub_changes(usb_hub_info_t * hub_info_param);


//int usb_add_hub_device(device_t *);



#endif
/**
 * @}
 */
