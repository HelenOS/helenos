/*
 * Copyright (c) 2014 Jan Kolarik
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

/** @file ar9271.c
 *
 * Driver for AR9271 USB WiFi dongle.
 *
 */

#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <usb/dev/poll.h>
#include <usb/debug.h>
#include <stdio.h>
#include <ddf/interrupt.h>
#include <nic.h>
#include <macros.h>

#include "ath_usb.h"
#include "wmi.h"
#include "hw.h"
#include "ar9271.h"

#define NAME "ar9271"
#define FIRMWARE_FILENAME "/drv/ar9271/ar9271.fw"

const usb_endpoint_description_t usb_ar9271_out_bulk_endpoint_description = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_VENDOR_SPECIFIC,
	.interface_subclass = 0x0,
	.interface_protocol = 0x0,
	.flags = 0
};

const usb_endpoint_description_t usb_ar9271_in_bulk_endpoint_description = {
	.transfer_type = USB_TRANSFER_BULK,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_VENDOR_SPECIFIC,
	.interface_subclass = 0x0,
	.interface_protocol = 0x0,
	.flags = 0
};

const usb_endpoint_description_t usb_ar9271_in_int_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_IN,
	.interface_class = USB_CLASS_VENDOR_SPECIFIC,
	.interface_subclass = 0x0,
	.interface_protocol = 0x0,
	.flags = 0
};

const usb_endpoint_description_t usb_ar9271_out_int_endpoint_description = {
	.transfer_type = USB_TRANSFER_INTERRUPT,
	.direction = USB_DIRECTION_OUT,
	.interface_class = USB_CLASS_VENDOR_SPECIFIC,
	.interface_subclass = 0x0,
	.interface_protocol = 0x0,
	.flags = 0
};

/* Array of endpoints expected on the device, NULL terminated. */
const usb_endpoint_description_t *endpoints[] = {
	&usb_ar9271_out_bulk_endpoint_description,
	&usb_ar9271_in_bulk_endpoint_description,
        &usb_ar9271_in_int_endpoint_description,
        &usb_ar9271_out_int_endpoint_description,
	NULL
};

/** Network interface options for AR9271 driver. */
static nic_iface_t ar9271_nic_iface;

/** Basic device operations for AR9271 NIC driver. */
static ddf_dev_ops_t ar9271_nic_dev_ops;

/** Basic driver operations for AR9271 NIC driver. */
static driver_ops_t ar9271_nic_driver_ops;

/* Callback when new device is to be controlled by this driver. */
static int ar9271_add_device(ddf_dev_t *);

static driver_ops_t ar9271_driver_ops = {
	.dev_add = ar9271_add_device
};

static driver_t ar9271_driver = {
	.name = NAME,
	.driver_ops = &ar9271_driver_ops
};

/* The default implementation callbacks */
static int ar9271_on_activating(nic_t *);
static int ar9271_on_stopping(nic_t *);
static void ar9271_send_frame(nic_t *, void *, size_t);

static int ar9271_init(ar9271_t *ar9271, usb_device_t *usb_device)
{
	ar9271->usb_device = usb_device;
	
	ath_t *ath_device = malloc(sizeof(ath_t));
	if (!ath_device) {
		usb_log_error("Failed to allocate memory for ath device "
		    "structure.\n");
		return ENOMEM;
	}
	
	int rc = ath_usb_init(ath_device, usb_device);
	if(rc != EOK) {
		free(ath_device);
		usb_log_error("Failed to initialize ath device.\n");
		return EINVAL;
	}
		
	/* HTC device structure initialization. */
	ar9271->htc_device = malloc(sizeof(htc_device_t));
	if(!ar9271->htc_device) {
		free(ath_device);
		usb_log_error("Failed to allocate memory for HTC device "
		    "structure.\n");
		return ENOMEM;
	}
	
	memset(ar9271->htc_device, 0, sizeof(htc_device_t));
        
	rc = htc_device_init(ath_device, ar9271->htc_device);
	if(rc != EOK) {
		free(ar9271->htc_device);
		free(ath_device);
		usb_log_error("Failed to initialize HTC device.\n");
		return EINVAL;
	}
	
	return EOK;
}

/**
 * Upload firmware to WiFi device.
 * 
 * @param ar9271 AR9271 device structure
 * 
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_upload_fw(ar9271_t *ar9271)
{
	int rc;
	
	usb_device_t *usb_device = ar9271->usb_device;
        
        /* TODO: Set by maximum packet size in pipe. */
	static const size_t MAX_TRANSFER_SIZE = 512;
	
	/* Load FW from file. */
	FILE *fw_file = fopen(FIRMWARE_FILENAME, "rb");
	if (fw_file == NULL) {
		usb_log_error("Failed opening file with firmware.\n");
		return ENOENT;
	}
	
	fseek(fw_file, 0, SEEK_END);
	uint64_t file_size = ftell(fw_file);
	fseek(fw_file, 0, SEEK_SET);
	
	void *fw_data = malloc(file_size);
	if(fw_data == NULL) {
		fclose(fw_file);
		usb_log_error("Failed allocating memory for firmware.\n");
		return ENOMEM;
	}

	fread(fw_data, file_size, 1, fw_file);
	fclose(fw_file);
	
	usb_log_info("Firmware loaded successfully.\n");
	
	/* Upload FW to device. */
	uint64_t remain_size = file_size;
	uint32_t current_addr = AR9271_FW_ADDRESS;
	uint8_t *current_data = fw_data;
	uint8_t *buffer = malloc(MAX_TRANSFER_SIZE);
	while(remain_size > 0) {
		size_t chunk_size = min(remain_size, MAX_TRANSFER_SIZE);
		memcpy(buffer, current_data, chunk_size);
		rc = usb_control_request_set(&usb_device->ctrl_pipe,
		    USB_REQUEST_TYPE_VENDOR,
		    USB_REQUEST_RECIPIENT_DEVICE,
		    AR9271_FW_DOWNLOAD,
		    uint16_host2usb(current_addr >> 8),
		    0, buffer, chunk_size);
		if(rc != EOK) {
			free(fw_data);
			free(buffer);
			usb_log_error("Error while uploading firmware. "
			    "Error: %d\n", rc);
			return rc;
		}
		
		remain_size -= chunk_size;
		current_addr += chunk_size;
		current_data += chunk_size;
	}
	
	free(fw_data);
	free(buffer);
	
	/* Send command that firmware is successfully uploaded. 
         * This should initiate creating confirmation message in
         * device side buffer which we will check in htc_check_ready function.
         */
	rc = usb_control_request_set(&usb_device->ctrl_pipe,
	    USB_REQUEST_TYPE_VENDOR,
	    USB_REQUEST_RECIPIENT_DEVICE,
	    AR9271_FW_DOWNLOAD_COMP,
	    uint16_host2usb(AR9271_FW_OFFSET >> 8),
	    0, NULL, 0);
	
	if(rc != EOK) {
		usb_log_error("IO error when sending fw upload confirmation "
                        "message.\n");
		return rc;
	}
	
	usb_log_info("Firmware uploaded successfully.\n");
	
	/* Wait until firmware is ready - wait for 1 second to be sure. */
	sleep(1);
	
	return rc;
}

/** Get MAC address of the AR9271 adapter
 *
 *  @param ar9271 The AR9271 device
 *  @param address The place to store the address
 *
 *  @return EOK if succeed, negative error code otherwise
 */
inline static void ar9271_hw_get_addr(ar9271_t *ar9271, nic_address_t *addr)
{
	assert(ar9271);
	assert(addr);
	
	// TODO
}

/** Force receiving all frames in the receive buffer
 *
 * @param nic NIC data
 */
static void ar9271_poll(nic_t *nic)
{
	assert(nic);
}

/** Set polling mode
 *
 * @param device  Device to set
 * @param mode    Mode to set
 * @param period  Period for NIC_POLL_PERIODIC
 *
 * @return EOK if succeed, ENOTSUP if the mode is not supported
 */
static int ar9271_poll_mode_change(nic_t *nic, nic_poll_mode_t mode,
    const struct timeval *period)
{
	assert(nic);
	return EOK;
}

/** Set multicast frames acceptance mode
 *
 * @param nic      NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_on_multicast_mode_change(nic_t *nic, 
    nic_multicast_mode_t mode, const nic_address_t *addr, size_t addr_cnt)
{
	assert(nic);
	return EOK;
}

/** Set unicast frames acceptance mode
 *
 * @param nic      NIC device to update
 * @param mode     Mode to set
 * @param addr     Address list (used in mode = NIC_MULTICAST_LIST)
 * @param addr_cnt Length of address list (used in mode = NIC_MULTICAST_LIST)
 *
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_on_unicast_mode_change(nic_t *nic, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_cnt)
{
	assert(nic);
	return EOK;
}

/** Set broadcast frames acceptance mode
 *
 * @param nic  NIC device to update
 * @param mode Mode to set
 *
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_on_broadcast_mode_change(nic_t *nic, 
    nic_broadcast_mode_t mode)
{
	assert(nic);
	return EOK;
}

/** Activate the device to receive and transmit frames
 *
 * @param nic NIC driver data
 *
 * @return EOK if activated successfully, negative error code otherwise
 */
static int ar9271_on_activating(nic_t *nic)
{
	assert(nic);
	return EOK;
}

/** Callback for NIC_STATE_STOPPED change
 *
 * @param nic NIC driver data
 *
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_on_stopping(nic_t *nic)
{
	assert(nic);
	return EOK;
}

/** Send frame
 *
 * @param nic    NIC driver data structure
 * @param data   Frame data
 * @param size   Frame size in bytes
 */
static void ar9271_send_frame(nic_t *nic, void *data, size_t size)
{
	assert(nic);
}

/** Create driver data structure.
 * 
 *  @param dev The device structure
 * 
 *  @return Intialized device data structure or NULL if error occured
 */
static ar9271_t *ar9271_create_dev_data(ddf_dev_t *dev)
{
	/* USB framework initialization. */
	usb_device_t *usb_device = calloc(1, sizeof(usb_device_t));
	if (usb_device == NULL) {
		usb_log_error("USB device structure allocation failed.\n");
		return NULL;
	}
	
	const char *err_msg = NULL;
	int rc = usb_device_init(usb_device, dev, endpoints, &err_msg);
	if(rc != EOK) {
		free(usb_device);
		usb_log_error("Failed to create USB device: %s, "
		    "ERR_NUM = %d\n", err_msg, rc);
		return NULL;
	}
	
	/* AR9271 structure initialization. */
	ar9271_t *ar9271 = malloc(sizeof(ar9271_t));
	if (!ar9271) {
		free(usb_device);
		usb_log_error("Failed to allocate memory for device "
		    "structure.\n");
		return NULL;
	}
	
	memset(ar9271, 0, sizeof(ar9271_t));
	
	rc = ar9271_init(ar9271, usb_device);
	if(rc != EOK) {
		free(ar9271);
		free(usb_device);
		usb_log_error("Failed to initialize AR9271 structure: %d\n", 
			rc);
		return NULL;
	}
	
	/* NIC framework initialization. */
	nic_t *nic = nic_create_and_bind(dev);
	if (!nic) {
		free(ar9271);
		free(usb_device);
		usb_log_error("Failed to create and bind NIC data.\n");
		return NULL;
	}
	
	nic_set_specific(nic, ar9271);
	nic_set_send_frame_handler(nic, ar9271_send_frame);
	nic_set_state_change_handlers(nic, ar9271_on_activating,
	    NULL, ar9271_on_stopping);
	nic_set_filtering_change_handlers(nic,
	    ar9271_on_unicast_mode_change, ar9271_on_multicast_mode_change,
	    ar9271_on_broadcast_mode_change, NULL, NULL);
	nic_set_poll_handlers(nic, ar9271_poll_mode_change, ar9271_poll);
	
	return ar9271;
}

/** Clean up the ar9271 device structure.
 *
 * @param dev  The device structure.
 */
static void ar9271_delete_dev_data(ar9271_t *ar9271)
{
	assert(ar9271);
	
	// TODO
}

/** Probe and initialize the newly added device.
 *
 * @param dev The device structure.
 *
 * @return EOK if succeed, negative error code otherwise
 */
static int ar9271_add_device(ddf_dev_t *dev)
{
	assert(dev);
	ddf_fun_t *fun;
	
	/* Allocate driver data for the device. */
	ar9271_t *ar9271 = ar9271_create_dev_data(dev);
	if (ar9271 == NULL) {
		usb_log_error("Unable to allocate device softstate.\n");
		return ENOMEM;
	}
	
	usb_log_info("HelenOS AR9271 device initialized.\n");
	
	/* Upload AR9271 firmware. */
	ar9271_upload_fw(ar9271);
	
	/* Initialize AR9271 HTC services. */
	int rc = htc_init(ar9271->htc_device);
	if(rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("HTC initialization failed.\n");
		return rc;
	}
	
	/* Initialize AR9271 HW. */
	rc = hw_init(ar9271);
	if(rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("HW initialization failed.\n");
		return rc;
	}
	
	nic_t *nic = nic_get_from_ddf_dev(dev);
	
	/* TODO: Report HW address here. */
	
	/* Create AR9271 function.*/
	fun = ddf_fun_create(nic_get_ddf_dev(nic), fun_exposed, "port0");
	if (fun == NULL) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("Failed creating device function.\n");
	}
	
	nic_set_ddf_fun(nic, fun);
	ddf_fun_set_ops(fun, &ar9271_nic_dev_ops);
	
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_fun_destroy(fun);
		ar9271_delete_dev_data(ar9271);
		usb_log_error("Failed binding device function.\n");
		return rc;
	}
	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK) {
		ddf_fun_unbind(fun);
		ar9271_delete_dev_data(ar9271);
		usb_log_error("Failed adding function to category.\n");
		return rc;
	}
	
	usb_log_info("HelenOS AR9271 added device.\n");
	
	return EOK;
}

int main(void)
{
	log_init(NAME);
	
	if (nic_driver_init(NAME) != EOK)
		return 1;
	
	nic_driver_implement(&ar9271_nic_driver_ops, &ar9271_nic_dev_ops,
	    &ar9271_nic_iface);

	usb_log_info("HelenOS AR9271 driver started.\n");

	return ddf_driver_main(&ar9271_driver);
}
