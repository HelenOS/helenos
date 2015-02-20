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

#include <ieee80211.h>
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

/* Callback when new device is to be controlled by this driver. */
static int ar9271_add_device(ddf_dev_t *);

/* IEEE802.11 callbacks */
static int ar9271_ieee80211_start(ieee80211_dev_t *ieee80211_dev);
static int ar9271_ieee80211_tx_handler(ieee80211_dev_t *ieee80211_dev,
	void *buffer, size_t buffer_size);

static driver_ops_t ar9271_driver_ops = {
	.dev_add = ar9271_add_device
};

static driver_t ar9271_driver = {
	.name = NAME,
	.driver_ops = &ar9271_driver_ops
};

static ieee80211_ops_t ar9271_ieee80211_ops = {
	.start = ar9271_ieee80211_start,
	.tx_handler = ar9271_ieee80211_tx_handler
};

static int ar9271_ieee80211_tx_handler(ieee80211_dev_t *ieee80211_dev,
	void *buffer, size_t buffer_size)
{
	/* TODO: Process TX message properly */
	
	size_t complete_size, offset;
	void *complete_buffer;
	int endpoint;
	
	ar9271_t *ar9271 = (ar9271_t *) ieee80211_dev->driver_data;
	
	ieee80211_header_t *ieee80211_header = (ieee80211_header_t *) buffer;
	
	if(ieee80211_is_data_frame(ieee80211_header)) {
		offset = sizeof(htc_frame_header_t);
		complete_size = buffer_size + offset;
		complete_buffer = malloc(complete_size);
		endpoint = ar9271->htc_device->endpoints.data_be_endpoint;
	} else {
		offset = sizeof(htc_tx_management_header_t) +
			sizeof(htc_frame_header_t);
		complete_size = buffer_size + offset;
		complete_buffer = malloc(complete_size);
		endpoint = ar9271->htc_device->endpoints.mgmt_endpoint;
	}
	
	/* Copy IEEE802.11 data to new allocated buffer with HTC headers. */
	memcpy(complete_buffer + offset, buffer, buffer_size);
	
	htc_send_data_message(ar9271->htc_device, complete_buffer,
		complete_size, endpoint);
	
	free(complete_buffer);
	
	return EOK;
}

static int ar9271_data_polling(void *arg)
{
	ar9271_t *ar9271 = (ar9271_t *) arg;
	size_t buffer_size = ar9271->ath_device->data_response_length;
	void *buffer = malloc(buffer_size);
	
	while(true) {
		int rc = 
		htc_read_data_message(ar9271->htc_device, buffer, buffer_size,
			NULL);
		usb_log_info("RC is %d.\n", rc);
		
		/* TODO: Process RX message */
	}
	
	return EOK;
}

static int ar9271_diag_polling(void *arg)
{
	ar9271_t *ar9271 = (ar9271_t *) arg;
	
	while(true) {
		uint32_t result;
		wmi_reg_read(ar9271->htc_device, 0x80F0, &result);
		usb_log_info("RX count: %x\n", result);
		wmi_reg_read(ar9271->htc_device, 0x8098, &result);
		usb_log_info("Beacon count: %x\n", result);
		sleep(1);
	}
	
	return EOK;
}

static int ar9271_register_polling_fibrils(ar9271_t *ar9271)
{
	/* Add data polling fibril. */
	fid_t fibril = fibril_create(ar9271_data_polling, ar9271);
	if (fibril == 0) {
		return ENOMEM;
	}
	fibril_add_ready(fibril);
	
	/* Add debug polling fibril. */
	fibril = fibril_create(ar9271_diag_polling, ar9271);
	if (fibril == 0) {
		return ENOMEM;
	}
	fibril_add_ready(fibril);
	
	return EOK;
}

static int ar9271_ieee80211_start(ieee80211_dev_t *ieee80211_dev)
{
	ar9271_t *ar9271 = (ar9271_t *) ieee80211_dev->driver_data;
	
	int rc = wmi_send_command(ar9271->htc_device, WMI_FLUSH_RECV, NULL, 0, 
		NULL);
	if(rc != EOK) {
		usb_log_error("Failed to flush receiving buffer.\n");
		return rc;
	}
	
	rc = hw_reset(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to do HW reset.\n");
		return rc;
	}
	
	uint16_t htc_mode = host2uint16_t_be(1);
	rc = wmi_send_command(ar9271->htc_device, WMI_SET_MODE, 
		(uint8_t *) &htc_mode, sizeof(htc_mode), NULL);
	if(rc != EOK) {
		usb_log_error("Failed to set HTC mode.\n");
		return rc;
	}
	
	rc = wmi_send_command(ar9271->htc_device, WMI_ATH_INIT, NULL, 0, 
		NULL);
	if(rc != EOK) {
		usb_log_error("Failed to send ath init command.\n");
		return rc;
	}
	
	rc = wmi_send_command(ar9271->htc_device, WMI_START_RECV, NULL, 0, 
		NULL);
	if(rc != EOK) {
		usb_log_error("Failed to send receiving init command.\n");
		return rc;
	}
	
	rc = hw_rx_init(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to initialize RX.\n");
		return rc;
	}
	
	rc = htc_init_new_vif(ar9271->htc_device);
	if(rc != EOK) {
		usb_log_error("Failed to initialize new VIF.\n");
		return rc;
	}
	
	rc = ar9271_register_polling_fibrils(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to register polling fibrils.\n");
		return rc;
	}
	
	return EOK;
}

static int ar9271_init(ar9271_t *ar9271, usb_device_t *usb_device)
{
	ar9271->usb_device = usb_device;
	
	ar9271->ath_device = calloc(1, sizeof(ath_t));
	if (!ar9271->ath_device) {
		usb_log_error("Failed to allocate memory for ath device "
		    "structure.\n");
		return ENOMEM;
	}
	
	int rc = ath_usb_init(ar9271->ath_device, usb_device);
	if(rc != EOK) {
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize ath device.\n");
		return rc;
	}
	
	/* IEEE 802.11 framework structure initialization. */
	ar9271->ieee80211_dev = calloc(1, sizeof(ieee80211_dev_t));
	if (!ar9271->ieee80211_dev) {
		free(ar9271->ath_device);
		usb_log_error("Failed to allocate memory for IEEE80211 device "
		    "structure.\n");
		return ENOMEM;
	}
	
	rc = ieee80211_device_init(ar9271->ieee80211_dev, ar9271, 
		ar9271->ddf_dev);
	if(rc != EOK) {
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize IEEE80211 device structure."
			"\n");
		return rc;
	}
		
	/* HTC device structure initialization. */
	ar9271->htc_device = calloc(1, sizeof(htc_device_t));
	if(!ar9271->htc_device) {
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to allocate memory for HTC device "
		    "structure.\n");
		return ENOMEM;
	}
	
	rc = htc_device_init(ar9271->ath_device, ar9271->ieee80211_dev, 
		ar9271->htc_device);
	if(rc != EOK) {
		free(ar9271->htc_device);
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize HTC device structure.\n");
		return rc;
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
	ar9271_t *ar9271 = calloc(1, sizeof(ar9271_t));
	if (!ar9271) {
		free(usb_device);
		usb_log_error("Failed to allocate memory for device "
		    "structure.\n");
		return NULL;
	}
	
	ar9271->ddf_dev = dev;
	
	rc = ar9271_init(ar9271, usb_device);
	if(rc != EOK) {
		free(ar9271);
		free(usb_device);
		usb_log_error("Failed to initialize AR9271 structure: %d\n", 
			rc);
		return NULL;
	}
	
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
	
	/* Initialize AR9271 IEEE 802.11 framework. */
	rc = ieee80211_init(ar9271->ieee80211_dev, &ar9271_ieee80211_ops);
	if(rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("Failed to initialize IEEE80211 framework.\n");
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
	
	usb_log_info("HelenOS AR9271 driver started.\n");

	return ddf_driver_main(&ar9271_driver);
}