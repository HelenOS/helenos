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
#include <errno.h>
#include <str_error.h>
#include "wmi.h"
#include "htc.h"
#include "nic/nic.h"
#include "ar9271.h"

/** HTC download pipes mapping.
 *
 * @param service_id Identification of WMI service.
 *
 * @return Number of pipe used for this service.
 *
 */
static inline uint8_t wmi_service_to_download_pipe(wmi_services_t service_id)
{
	return (service_id == WMI_CONTROL_SERVICE) ? 3 : 2;
}

/** HTC upload pipes mapping.
 *
 * @param service_id Identification of WMI service.
 *
 * @return Number of pipe used for this service.
 *
 */
static inline uint8_t wmi_service_to_upload_pipe(wmi_services_t service_id)
{
	return (service_id == WMI_CONTROL_SERVICE) ? 4 : 1;
}

errno_t htc_init_new_vif(htc_device_t *htc_device)
{
	htc_vif_msg_t vif_msg;
	htc_sta_msg_t sta_msg;
	
	memset(&vif_msg, 0, sizeof(htc_vif_msg_t));
	memset(&sta_msg, 0, sizeof(htc_sta_msg_t));
	
	nic_address_t addr;
	nic_t *nic =
	    nic_get_from_ddf_dev(ieee80211_get_ddf_dev(htc_device->ieee80211_dev));
	nic_query_address(nic, &addr);
	
	memcpy(&vif_msg.addr, &addr.address, ETH_ADDR);
	memcpy(&sta_msg.addr, &addr.address, ETH_ADDR);
	
	ieee80211_operating_mode_t op_mode =
	    ieee80211_query_current_op_mode(htc_device->ieee80211_dev);
	
	switch (op_mode) {
	case IEEE80211_OPMODE_ADHOC:
		vif_msg.op_mode = HTC_OPMODE_ADHOC;
		break;
	case IEEE80211_OPMODE_AP:
		vif_msg.op_mode = HTC_OPMODE_AP;
		break;
	case IEEE80211_OPMODE_MESH:
		vif_msg.op_mode = HTC_OPMODE_MESH;
		break;
	case IEEE80211_OPMODE_STATION:
		vif_msg.op_mode = HTC_OPMODE_STATION;
		break;
	}
	
	vif_msg.index = 0;
	vif_msg.rts_thres = host2uint16_t_be(HTC_RTS_THRESHOLD);
	
	wmi_send_command(htc_device, WMI_VAP_CREATE, (uint8_t *) &vif_msg,
	    sizeof(vif_msg), NULL);
	
	sta_msg.is_vif_sta = 1;
	sta_msg.max_ampdu = host2uint16_t_be(0xFFFF);
	sta_msg.sta_index = 0;
	sta_msg.vif_index = 0;
	
	wmi_send_command(htc_device, WMI_NODE_CREATE, (uint8_t *) &sta_msg,
	    sizeof(sta_msg), NULL);
	
	/* Write first 4 bytes of MAC address. */
	uint32_t id0;
	memcpy(&id0, &addr.address, 4);
	id0 = host2uint32_t_le(id0);
	wmi_reg_write(htc_device, AR9271_STATION_ID0, id0);
	
	/* Write last 2 bytes of MAC address (and preserve existing data). */
	uint32_t id1;
	wmi_reg_read(htc_device, AR9271_STATION_ID1, &id1);
	
	uint16_t id1_addr;
	memcpy(&id1_addr, &addr.address[4], 2);
	id1 = (id1 & ~AR9271_STATION_ID1_MASK) | host2uint16_t_le(id1_addr);
	wmi_reg_write(htc_device, AR9271_STATION_ID1, id1);
	
	return EOK;
}

static void htc_config_frame_header(htc_frame_header_t *header,
    size_t buffer_size, uint8_t endpoint_id)
{
	memset(header, 0, sizeof(htc_frame_header_t));
	
	header->endpoint_id = endpoint_id;
	header->payload_length =
	    host2uint16_t_be(buffer_size - sizeof(htc_frame_header_t));
}

/** Send control HTC message to USB device.
 *
 * @param htc_device  HTC device structure.
 * @param buffer      Buffer with data to be sent to USB device
 *                    (without HTC frame header).
 * @param buffer_size Size of buffer (including HTC frame header).
 * @param endpoint_id Destination endpoint.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_send_control_message(htc_device_t *htc_device, void *buffer,
    size_t buffer_size, uint8_t endpoint_id)
{
	htc_config_frame_header((htc_frame_header_t *) buffer, buffer_size,
	    endpoint_id);
	
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->send_ctrl_message(ath_device, buffer,
	    buffer_size);
}

/** Send data HTC message to USB device.
 *
 * @param htc_device  HTC device structure.
 * @param buffer      Buffer with data to be sent to USB device
 *                    (without HTC frame header).
 * @param buffer_size Size of buffer (including HTC frame header).
 * @param endpoint_id Destination endpoint.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_send_data_message(htc_device_t *htc_device, void *buffer,
    size_t buffer_size, uint8_t endpoint_id)
{
	htc_config_frame_header((htc_frame_header_t *) buffer, buffer_size,
	    endpoint_id);
	
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->send_data_message(ath_device, buffer,
	    buffer_size);
}

/** Read HTC data message from USB device.
 *
 * @param htc_device       HTC device structure.
 * @param buffer           Buffer where data from USB device
 *                         will be stored.
 * @param buffer_size      Size of buffer.
 * @param transferred_size Real size of read data.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_read_data_message(htc_device_t *htc_device, void *buffer,
    size_t buffer_size, size_t *transferred_size)
{
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->read_data_message(ath_device, buffer,
	    buffer_size, transferred_size);
}

/** Read HTC control message from USB device.
 *
 * @param htc_device       HTC device structure.
 * @param buffer           Buffer where data from USB device
 *                         will be stored.
 * @param buffer_size      Size of buffer.
 * @param transferred_size Real size of read data.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_read_control_message(htc_device_t *htc_device, void *buffer,
    size_t buffer_size, size_t *transferred_size)
{
	ath_t *ath_device = htc_device->ath_device;
	
	return ath_device->ops->read_ctrl_message(ath_device, buffer,
	    buffer_size, transferred_size);
}

/** Initialize HTC service.
 *
 * @param htc_device           HTC device structure.
 * @param service_id           Identification of WMI service.
 * @param response_endpoint_no HTC endpoint to be used for
 *                             this service.
 *
 * @return EOK if succeed, EINVAL when failed to connect service,
 *         error code otherwise.
 *
 */
static errno_t htc_connect_service(htc_device_t *htc_device,
    wmi_services_t service_id, int *response_endpoint_no)
{
	size_t buffer_size = sizeof(htc_frame_header_t) +
	    sizeof(htc_service_msg_t);
	void *buffer = malloc(buffer_size);
	memset(buffer, 0, buffer_size);
	
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
	errno_t rc = htc_send_control_message(htc_device, buffer, buffer_size,
	    htc_device->endpoints.ctrl_endpoint);
	if (rc != EOK) {
		free(buffer);
		usb_log_error("Failed to send HTC message. Error: %s\n", str_error_name(rc));
		return rc;
	}
	
	free(buffer);
	
	buffer_size = htc_device->ath_device->ctrl_response_length;
	buffer = malloc(buffer_size);
	
	/* Read response from device. */
	rc = htc_read_control_message(htc_device, buffer, buffer_size, NULL);
	if (rc != EOK) {
		free(buffer);
		usb_log_error("Failed to receive HTC service connect response. "
		    "Error: %s\n", str_error_name(rc));
		return rc;
	}
	
	htc_service_resp_msg_t *response_message = (htc_service_resp_msg_t *)
	    ((void *) buffer + sizeof(htc_frame_header_t));
	
	/*
	 * If service was successfully connected,
	 * write down HTC endpoint number that will
	 * be used for communication.
	 */
	if (response_message->status == HTC_SERVICE_SUCCESS) {
		*response_endpoint_no = response_message->endpoint_id;
		rc = EOK;
	} else {
		usb_log_error("Failed to connect HTC service. "
		    "Message status: %d\n", response_message->status);
		rc = EINVAL;
	}
	
	free(buffer);
	
	return rc;
}

/** HTC credits initialization message.
 *
 * @param htc_device HTC device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t htc_config_credits(htc_device_t *htc_device)
{
	size_t buffer_size = sizeof(htc_frame_header_t) +
	    sizeof(htc_config_msg_t);
	void *buffer = malloc(buffer_size);
	htc_config_msg_t *config_message = (htc_config_msg_t *)
	    ((void *) buffer + sizeof(htc_frame_header_t));
	
	config_message->message_id = 
	    host2uint16_t_be(HTC_MESSAGE_CONFIG);
	/* Magic number to initialize device. */
	config_message->credits = 33;
	config_message->pipe_id = 1;
	
	/* Send HTC message. */
	errno_t rc = htc_send_control_message(htc_device, buffer, buffer_size,
	    htc_device->endpoints.ctrl_endpoint);
	if (rc != EOK) {
		free(buffer);
		usb_log_error("Failed to send HTC config message. "
		    "Error: %s\n", str_error_name(rc));
		return rc;
	}
	
	free(buffer);
	
	buffer_size = htc_device->ath_device->ctrl_response_length;
	buffer = malloc(buffer_size);
	
	/* Check response from device. */
	rc = htc_read_control_message(htc_device, buffer, buffer_size, NULL);
	if (rc != EOK) {
		usb_log_error("Failed to receive HTC config response message. "
		    "Error: %s\n", str_error_name(rc));
	}
	
	free(buffer);
	
	return rc;
}

/** HTC setup complete confirmation message.
 *
 * @param htc_device HTC device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t htc_complete_setup(htc_device_t *htc_device)
{
	size_t buffer_size = sizeof(htc_frame_header_t) +
	    sizeof(htc_setup_complete_msg_t);
	void *buffer = malloc(buffer_size);
	htc_setup_complete_msg_t *complete_message =
	    (htc_setup_complete_msg_t *)
	    ((void *) buffer + sizeof(htc_frame_header_t));
	
	complete_message->message_id =
	    host2uint16_t_be(HTC_MESSAGE_SETUP_COMPLETE);
	
	/* Send HTC message. */
	errno_t rc = htc_send_control_message(htc_device, buffer, buffer_size,
	    htc_device->endpoints.ctrl_endpoint);
	if (rc != EOK)
		usb_log_error("Failed to send HTC setup complete message. "
		    "Error: %s\n", str_error_name(rc));
	
	free(buffer);
	
	return rc;
}

/** Try to fetch ready message from device.
 *
 * Checks that firmware was successfully loaded on device side.
 *
 * @param htc_device HTC device structure.
 *
 * @return EOK if succeed, EINVAL if response error,
 *         error code otherwise.
 *
 */
static errno_t htc_check_ready(htc_device_t *htc_device)
{
	size_t buffer_size = htc_device->ath_device->ctrl_response_length;
	void *buffer = malloc(buffer_size);
	
	/* Read response from device. */
	errno_t rc = htc_read_control_message(htc_device, buffer, buffer_size,
	    NULL);
	if (rc != EOK) {
		free(buffer);
		usb_log_error("Failed to receive HTC check ready message. "
		    "Error: %s\n", str_error_name(rc));
		return rc;
	}
	
	uint16_t *message_id = (uint16_t *) ((void *) buffer +
	    sizeof(htc_frame_header_t));
	if (uint16_t_be2host(*message_id) == HTC_MESSAGE_READY)
		rc = EOK;
	else
		rc = EINVAL;
	
	free(buffer);
	
	return rc;
}

/** Initialize HTC device structure.
 *
 * @param ath_device Atheros WiFi device connected
 *                   with this HTC device.
 * @param htc_device HTC device structure to be initialized.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_device_init(ath_t *ath_device, ieee80211_dev_t *ieee80211_dev,
    htc_device_t *htc_device)
{
	fibril_mutex_initialize(&htc_device->rx_lock);
	fibril_mutex_initialize(&htc_device->tx_lock);
	
	htc_device->endpoints.ctrl_endpoint = 0;
	
	htc_device->ath_device = ath_device;
	htc_device->ieee80211_dev = ieee80211_dev;
	
	return EOK;
}

/** HTC communication initalization.
 *
 * @param htc_device HTC device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t htc_init(htc_device_t *htc_device)
{
	/* First check ready message in device. */
	errno_t rc = htc_check_ready(htc_device);
	if (rc != EOK) {
		usb_log_error("Device is not in ready state after loading "
		    "firmware.\n");
		return rc;
	}
	
	/*
	 * HTC services initialization start.
	 */
	rc = htc_connect_service(htc_device, WMI_CONTROL_SERVICE,
	    &htc_device->endpoints.wmi_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing WMI service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_BEACON_SERVICE,
	    &htc_device->endpoints.beacon_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing beacon service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_CAB_SERVICE,
	    &htc_device->endpoints.cab_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing CAB service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_UAPSD_SERVICE,
	    &htc_device->endpoints.uapsd_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing UAPSD service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_MGMT_SERVICE,
	    &htc_device->endpoints.mgmt_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing MGMT service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_BE_SERVICE,
	    &htc_device->endpoints.data_be_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing data best effort "
		    "service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_BK_SERVICE,
	    &htc_device->endpoints.data_bk_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing data background "
		    "service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_VIDEO_SERVICE,
	    &htc_device->endpoints.data_video_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing data video service.\n");
		return rc;
	}
	
	rc = htc_connect_service(htc_device, WMI_DATA_VOICE_SERVICE,
	    &htc_device->endpoints.data_voice_endpoint);
	if (rc != EOK) {
		usb_log_error("Error while initalizing data voice service.\n");
		return rc;
	}
	
	/*
	 * HTC services initialization end.
	 */
	
	/* Credits initialization message. */
	rc = htc_config_credits(htc_device);
	if (rc != EOK) {
		usb_log_error("Failed to send HTC config message.\n");
		return rc;
	}
	
	/* HTC setup complete confirmation message. */
	rc = htc_complete_setup(htc_device);
	if (rc != EOK) {
		usb_log_error("Failed to send HTC complete setup message.\n");
		return rc;
	}
	
	usb_log_info("HTC services initialization finished successfully.\n");
	
	return EOK;
}
