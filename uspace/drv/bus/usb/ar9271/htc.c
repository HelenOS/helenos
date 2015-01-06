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

/** @file htc.c
 *
 * Implementation of Atheros HTC communication.
 *
 */

#include <usb/debug.h>
#include <byteorder.h>

#include "wmi.h"
#include "htc.h"

#define MAX_RESP_LEN 64

/**
 * HTC download pipes mapping.
 * 
 * @param service_id Identification of WMI service.
 * 
 * @return Number of pipe used for this service.
 */
static inline uint8_t wmi_service_to_download_pipe(wmi_services_t service_id)
{
	switch(service_id) {
		case WMI_CONTROL_SERVICE:
			return 3;
		default:
			return 2;
	}
}

/**
 * HTC upload pipes mapping.
 * 
 * @param service_id Identification of WMI service.
 * 
 * @return Number of pipe used for this service.
 */
static inline uint8_t wmi_service_to_upload_pipe(wmi_services_t service_id)
{
	switch(service_id) {
		case WMI_CONTROL_SERVICE:
			return 4;
		default:
                        return 1;
	}
}

/**
 * Send HTC message to USB device.
 * 
 * @param htc_device HTC device structure.
 * @param buffer Buffer with data to be sent to USB device (without HTC frame
 *              header).
 * @param buffer_size Size of buffer (including HTC frame header).
 * @param endpoint_id Destination endpoint.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int htc_send_message(htc_device_t *htc_device, void *buffer, 
	size_t buffer_size, uint8_t endpoint_id)
{
	htc_frame_header_t *htc_header = (htc_frame_header_t *) buffer;
	htc_header->endpoint_id = endpoint_id;
	htc_header->flags = 0;
	htc_header->payload_length = host2uint16_t_be(buffer_size);
	
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->send_ctrl_message(ath_device, buffer, 
		buffer_size);
}

/**
 * Read HTC message from USB device.
 * 
 * @param htc_device HTC device structure.
 * @param buffer Buffer where data from USB device will be stored.
 * @param buffer_size Size of buffer.
 * @param transferred_size Real size of read data.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int htc_read_message(htc_device_t *htc_device, void *buffer, 
	size_t buffer_size, size_t *transferred_size)
{
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->read_ctrl_message(ath_device, buffer, 
		buffer_size, transferred_size);
}

/**
 * Initialize HTC service.
 * 
 * @param htc_device HTC device structure.
 * @param service_id Identification of WMI service.
 * @param response_endpoint_no HTC endpoint to be used for this service.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int htc_connect_service(htc_device_t *htc_device, 
    wmi_services_t service_id, int *response_endpoint_no)
{
	size_t buffer_size = sizeof(htc_frame_header_t) + 
		sizeof(htc_service_msg_t);
	void *buffer = malloc(buffer_size);
	
	/* Fill service message structure. */
	htc_service_msg_t *service_message = (htc_service_msg_t *)
		((void *) buffer + sizeof(htc_frame_header_t));
	service_message->service_id = 
		host2uint16_t_be(service_id);
	service_message->message_id = 
		host2uint16_t_be(HTC_MESSAGE_CONNECT_SERVICE);
	service_message->download_pipe_id = 
		wmi_service_to_download_pipe(service_id);
	service_message->upload_pipe_id = 
		wmi_service_to_upload_pipe(service_id);
	service_message->connection_flags = 0;
	
	/* Send HTC message. */
	int rc = htc_send_message(htc_device, buffer, buffer_size,
		htc_device->endpoints.ctrl_endpoint);
	if(rc != EOK) {
		usb_log_error("Failed to send HTC message. Error: %d\n", rc);
		return rc;
	}
	
	free(buffer);
	
	buffer_size = MAX_RESP_LEN;
	buffer = malloc(buffer_size);
	
	/* Read response from device. */
	rc = htc_read_message(htc_device, buffer, buffer_size, NULL);
	if(rc != EOK) {
		usb_log_error("Failed to receive HTC service connect response. "
			"Error: %d\n", rc);
		return rc;
	}
	
	htc_service_resp_msg_t *response_message = (htc_service_resp_msg_t *)
		((void *) buffer + sizeof(htc_frame_header_t));

	/* If service was successfully connected, write down HTC endpoint number
	 * that will be used for communication. */
	if(response_message->status == HTC_SERVICE_SUCCESS) {
		*response_endpoint_no = response_message->endpoint_id;
		return EOK;
	} else {
		usb_log_error("Failed to connect HTC service. "
			"Message status: %d\n", response_message->status);
		return rc;
        }
}

/**
 * HTC credits initialization message.
 * 
 * @param htc_device HTC device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int htc_config_credits(htc_device_t *htc_device)
{
	size_t buffer_size = sizeof(htc_frame_header_t) + 50;
	void *buffer = malloc(buffer_size);
	htc_config_msg_t *config_message = (htc_config_msg_t *)
		((void *) buffer + sizeof(htc_frame_header_t));
	
	config_message->message_id = 
		host2uint16_t_be(HTC_MESSAGE_CONFIG);
	/* Magic number to initialize device. */
	config_message->credits = 33;
	config_message->pipe_id = 1;

	int rc = htc_send_message(htc_device, buffer, buffer_size,
		htc_device->endpoints.ctrl_endpoint);
	if(rc != EOK) {
		usb_log_error("Failed to send HTC config message. "
			"Error: %d\n", rc);
		return rc;
	}
	
	free(buffer);
	
	buffer_size = MAX_RESP_LEN;
	buffer = malloc(buffer_size);

	/* Check response from device. */
	rc = htc_read_message(htc_device, buffer, buffer_size, NULL);
	if(rc != EOK) {
		usb_log_error("Failed to receive HTC config response message. "
			"Error: %d\n", rc);
		return rc;
	}

	return rc;
}

/**
 * HTC setup complete confirmation message.
 * 
 * @param htc_device HTC device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int htc_complete_setup(htc_device_t *htc_device)
{
	size_t buffer_size = sizeof(htc_frame_header_t) + 50;
	void *buffer = malloc(buffer_size);
	htc_setup_complete_msg_t *complete_message = 
		(htc_setup_complete_msg_t *)
		((void *) buffer + sizeof(htc_frame_header_t));
	
	complete_message->message_id = 
		host2uint16_t_be(HTC_MESSAGE_SETUP_COMPLETE);

	int rc = htc_send_message(htc_device, buffer, buffer_size, 
		htc_device->endpoints.ctrl_endpoint);
	if(rc != EOK) {
		usb_log_error("Failed to send HTC setup complete message. "
			"Error: %d\n", rc);
		return rc;
	}

	return rc;
}

/**
 * Try to fetch ready message from device.
 * Checks that firmware was successfully loaded on device side.
 * 
 * @param htc_device HTC device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int htc_check_ready(htc_device_t *htc_device)
{
	size_t buffer_size = MAX_RESP_LEN;
	void *buffer = malloc(buffer_size);

	int rc = htc_read_message(htc_device, buffer, buffer_size, NULL);
	if(rc != EOK) {
		usb_log_error("Failed to receive HTC check ready message. "
			"Error: %d\n", rc);
		return rc;
	}

	uint16_t *message_id = (uint16_t *) ((void *) buffer + 
		sizeof(htc_frame_header_t));
	if(uint16_t_be2host(*message_id) == HTC_MESSAGE_READY) {
		return EOK;
	} else {
		return EINVAL;
	}
}

/**
 * Initialize HTC device structure.
 * 
 * @param ath_device Atheros WiFi device connected with this HTC device.
 * @param htc_device HTC device structure to be initialized.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int htc_device_init(ath_t *ath_device, htc_device_t *htc_device)
{
	if(ath_device == NULL || htc_device == NULL) {
		return EINVAL;
	}
	
	fibril_mutex_initialize(&htc_device->rx_lock);
	fibril_mutex_initialize(&htc_device->tx_lock);
	
	htc_device->endpoints.ctrl_endpoint = 0;
	
	htc_device->ath_device = ath_device;
	
	return EOK;
}

/**
 * HTC communication initalization.
 * 
 * @param htc_device HTC device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int htc_init(htc_device_t *htc_device)
{
	/* First check ready message in device. */
	int rc = htc_check_ready(htc_device);
	if(rc != EOK) {
		usb_log_error("Device is not in ready state after loading "
			"firmware.\n");
		return rc;
	}

	/* 
	 * HTC services initialization start.
	 * 
	 */
	
	rc = htc_connect_service(htc_device, WMI_CONTROL_SERVICE,
	    &htc_device->endpoints.wmi_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing WMI service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_BEACON_SERVICE,
	    &htc_device->endpoints.beacon_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing beacon service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_CAB_SERVICE,
	    &htc_device->endpoints.cab_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing CAB service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_UAPSD_SERVICE,
	    &htc_device->endpoints.uapsd_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing UAPSD service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_MGMT_SERVICE,
	    &htc_device->endpoints.mgmt_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing MGMT service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_BE_SERVICE,
	    &htc_device->endpoints.data_be_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing data best effort "
		    "service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_DATA_BK_SERVICE,
	    &htc_device->endpoints.data_bk_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing data background "
		    "service.\n");
		return rc;
	}

	rc = htc_connect_service(htc_device, WMI_DATA_VIDEO_SERVICE,
	    &htc_device->endpoints.data_video_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing data video service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_VOICE_SERVICE,
	    &htc_device->endpoints.data_voice_endpoint);
	if(rc != EOK) {
		usb_log_error("Error while initalizing data voice service.\n");
		return rc;
	}

	/* 
	 * HTC services initialization end.
	 * 
	 */

	/* Credits initialization message. */
	rc = htc_config_credits(htc_device);
	if(rc != EOK) {
		usb_log_error("Failed to send HTC config message.\n");
		return rc;
	}

	/* HTC setup complete confirmation message. */
	rc = htc_complete_setup(htc_device);
	if(rc != EOK) {
		usb_log_error("Failed to send HTC complete setup message.\n");
		return rc;
	}

	usb_log_info("HTC services initialization finished successfully.\n");
	
	return EOK;
}