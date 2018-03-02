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

#include <async.h>
#include <ieee80211.h>
#include <usb/classes/classes.h>
#include <usb/dev/request.h>
#include <usb/dev/poll.h>
#include <usb/debug.h>
#include <stdio.h>
#include <ddf/interrupt.h>
#include <errno.h>
#include <str_error.h>
#include <nic.h>
#include <macros.h>
#include "ath_usb.h"
#include "wmi.h"
#include "hw.h"
#include "ar9271.h"

#define NAME  "ar9271"
#define FIRMWARE_FILENAME  "/drv/ar9271/ar9271.fw"

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
static errno_t ar9271_add_device(ddf_dev_t *);

/* IEEE 802.11 callbacks */
static errno_t ar9271_ieee80211_start(ieee80211_dev_t *);
static errno_t ar9271_ieee80211_tx_handler(ieee80211_dev_t *, void *, size_t);
static errno_t ar9271_ieee80211_set_freq(ieee80211_dev_t *, uint16_t);
static errno_t ar9271_ieee80211_bssid_change(ieee80211_dev_t *, bool);
static errno_t ar9271_ieee80211_key_config(ieee80211_dev_t *, ieee80211_key_config_t *,
    bool);

static driver_ops_t ar9271_driver_ops = {
	.dev_add = ar9271_add_device
};

static driver_t ar9271_driver = {
	.name = NAME,
	.driver_ops = &ar9271_driver_ops
};

static ieee80211_ops_t ar9271_ieee80211_ops = {
	.start = ar9271_ieee80211_start,
	.tx_handler = ar9271_ieee80211_tx_handler,
	.set_freq = ar9271_ieee80211_set_freq,
	.bssid_change = ar9271_ieee80211_bssid_change,
	.key_config = ar9271_ieee80211_key_config
};

static ieee80211_iface_t ar9271_ieee80211_iface;

static errno_t ar9271_get_device_info(ddf_fun_t *, nic_device_info_t *);
static errno_t ar9271_get_cable_state(ddf_fun_t *, nic_cable_state_t *);
static errno_t ar9271_get_operation_mode(ddf_fun_t *, int *, nic_channel_mode_t *,
    nic_role_t *);

static nic_iface_t ar9271_ieee80211_nic_iface = {
	.get_device_info = &ar9271_get_device_info,
	.get_cable_state = &ar9271_get_cable_state,
	.get_operation_mode = &ar9271_get_operation_mode
};

static ddf_dev_ops_t ar9271_ieee80211_dev_ops;

/** Get device information.
 *
 */
static errno_t ar9271_get_device_info(ddf_fun_t *dev, nic_device_info_t *info)
{
	assert(dev);
	assert(info);

	memset(info, 0, sizeof(nic_device_info_t));

	info->vendor_id = 0x0cf3;
	info->device_id = 0x9271;
	str_cpy(info->vendor_name, NIC_VENDOR_MAX_LENGTH,
	    "Atheros Communications, Inc.");
	str_cpy(info->model_name, NIC_MODEL_MAX_LENGTH,
	    "AR9271");

	return EOK;
}

/** Get cable state.
 *
 */
static errno_t ar9271_get_cable_state(ddf_fun_t *fun, nic_cable_state_t *state)
{
	*state = NIC_CS_PLUGGED;

	return EOK;
}

/** Get operation mode of the device.
 *
 */
static errno_t ar9271_get_operation_mode(ddf_fun_t *fun, int *speed,
    nic_channel_mode_t *duplex, nic_role_t *role)
{
	*duplex = NIC_CM_FULL_DUPLEX;
	*speed = 10;
	*role = NIC_ROLE_UNKNOWN;

	return EOK;
}

/** Set multicast frames acceptance mode.
 *
 */
static errno_t ar9271_on_multicast_mode_change(nic_t *nic,
    nic_multicast_mode_t mode, const nic_address_t *addr, size_t addr_cnt)
{
	switch (mode) {
	case NIC_MULTICAST_BLOCKED:
		/* TODO */
		break;
	case NIC_MULTICAST_LIST:
		/* TODO */
		break;
	case NIC_MULTICAST_PROMISC:
		/* TODO */
		break;
	default:
		return ENOTSUP;
	}

	return EOK;
}

/** Set unicast frames acceptance mode.
 *
 */
static errno_t ar9271_on_unicast_mode_change(nic_t *nic, nic_unicast_mode_t mode,
    const nic_address_t *addr, size_t addr_cnt)
{
	switch (mode) {
	case NIC_UNICAST_BLOCKED:
		/* TODO */
		break;
	case NIC_UNICAST_DEFAULT:
		/* TODO */
		break;
	case NIC_UNICAST_LIST:
		/* TODO */
		break;
	case NIC_UNICAST_PROMISC:
		/* TODO */
		break;
	default:
		return ENOTSUP;
	}

	return EOK;
}

/** Set broadcast frames acceptance mode.
 *
 */
static errno_t ar9271_on_broadcast_mode_change(nic_t *nic,
    nic_broadcast_mode_t mode)
{
	switch (mode) {
	case NIC_BROADCAST_BLOCKED:
		/* TODO */
		break;
	case NIC_BROADCAST_ACCEPTED:
		/* TODO */
		break;
	default:
		return ENOTSUP;
	}

	return EOK;
}

static bool ar9271_rx_status_error(uint8_t status)
{
	return (status & AR9271_RX_ERROR_PHY) || (status & AR9271_RX_ERROR_CRC);
}

static errno_t ar9271_data_polling(void *arg)
{
	assert(arg);

	ar9271_t *ar9271 = (ar9271_t *) arg;

	size_t buffer_size = ar9271->ath_device->data_response_length;
	void *buffer = malloc(buffer_size);

	while (true) {
		size_t transferred_size;
		if (htc_read_data_message(ar9271->htc_device,
		    buffer, buffer_size, &transferred_size) == EOK) {
			size_t strip_length =
			    sizeof(ath_usb_data_header_t) +
			    sizeof(htc_frame_header_t) +
			    sizeof(htc_rx_status_t);

			if (transferred_size < strip_length)
				continue;

			ath_usb_data_header_t *data_header =
			    (ath_usb_data_header_t *) buffer;

			/* Invalid packet. */
			if (data_header->tag != uint16_t_le2host(RX_TAG))
				continue;

			htc_rx_status_t *rx_status =
			    (htc_rx_status_t *) ((void *) buffer +
			    sizeof(ath_usb_data_header_t) +
			    sizeof(htc_frame_header_t));

			uint16_t data_length =
			    uint16_t_be2host(rx_status->data_length);

			int16_t payload_length =
			    transferred_size - strip_length;

			if (payload_length - data_length < 0)
				continue;

			if (ar9271_rx_status_error(rx_status->status))
				continue;

			void *strip_buffer = buffer + strip_length;

			ieee80211_rx_handler(ar9271->ieee80211_dev,
			    strip_buffer,
			    payload_length);
		}
	}

	free(buffer);

	return EOK;
}

/** IEEE 802.11 handlers.
 *
 */
static errno_t ar9271_ieee80211_set_freq(ieee80211_dev_t *ieee80211_dev,
    uint16_t freq)
{
	assert(ieee80211_dev);

	ar9271_t *ar9271 = (ar9271_t *) ieee80211_get_specific(ieee80211_dev);

	wmi_send_command(ar9271->htc_device, WMI_DISABLE_INTR, NULL, 0, NULL);
	wmi_send_command(ar9271->htc_device, WMI_DRAIN_TXQ_ALL, NULL, 0, NULL);
	wmi_send_command(ar9271->htc_device, WMI_STOP_RECV, NULL, 0, NULL);

	errno_t rc = hw_freq_switch(ar9271, freq);
	if (rc != EOK) {
		usb_log_error("Failed to HW switch frequency.\n");
		return rc;
	}

	wmi_send_command(ar9271->htc_device, WMI_START_RECV, NULL, 0, NULL);

	rc = hw_rx_init(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to initialize RX.\n");
		return rc;
	}

	uint16_t htc_mode = host2uint16_t_be(1);
	wmi_send_command(ar9271->htc_device, WMI_SET_MODE,
	    (uint8_t *) &htc_mode, sizeof(htc_mode), NULL);
	wmi_send_command(ar9271->htc_device, WMI_ENABLE_INTR, NULL, 0, NULL);

	return EOK;
}

static errno_t ar9271_ieee80211_bssid_change(ieee80211_dev_t *ieee80211_dev,
    bool connected)
{
	assert(ieee80211_dev);

	ar9271_t *ar9271 = (ar9271_t *) ieee80211_get_specific(ieee80211_dev);

	if (connected) {
		nic_address_t bssid;
		ieee80211_query_bssid(ieee80211_dev, &bssid);

		htc_sta_msg_t sta_msg;
		memset(&sta_msg, 0, sizeof(htc_sta_msg_t));
		sta_msg.is_vif_sta = 0;
		sta_msg.max_ampdu =
		    host2uint16_t_be(1 << IEEE80211_MAX_AMPDU_FACTOR);
		sta_msg.sta_index = 1;
		sta_msg.vif_index = 0;
		memcpy(&sta_msg.addr, bssid.address, ETH_ADDR);

		wmi_send_command(ar9271->htc_device, WMI_NODE_CREATE,
		    (uint8_t *) &sta_msg, sizeof(sta_msg), NULL);

		htc_rate_msg_t rate_msg;
		memset(&rate_msg, 0, sizeof(htc_rate_msg_t));
		rate_msg.sta_index = 1;
		rate_msg.is_new = 1;
		rate_msg.legacy_rates_count = ARRAY_SIZE(ieee80211bg_data_rates);
		memcpy(&rate_msg.legacy_rates,
		    ieee80211bg_data_rates,
		    ARRAY_SIZE(ieee80211bg_data_rates));

		wmi_send_command(ar9271->htc_device, WMI_RC_RATE_UPDATE,
		    (uint8_t *) &rate_msg, sizeof(rate_msg), NULL);

		hw_set_rx_filter(ar9271, true);
	} else {
		uint8_t station_id = 1;
		wmi_send_command(ar9271->htc_device, WMI_NODE_REMOVE,
		    &station_id, sizeof(station_id), NULL);

		hw_set_rx_filter(ar9271, false);
	}

	hw_set_bssid(ar9271);

	return EOK;
}

static errno_t ar9271_ieee80211_key_config(ieee80211_dev_t *ieee80211_dev,
    ieee80211_key_config_t *key_conf, bool insert)
{
	assert(ieee80211_dev);

	ar9271_t *ar9271 = (ar9271_t *) ieee80211_get_specific(ieee80211_dev);

	if(insert) {
		assert(key_conf);

		uint32_t key[5];
		uint32_t key_type;
		uint32_t reg_ptr, mic_reg_ptr;
		void *data_start;

		nic_address_t bssid;
		ieee80211_query_bssid(ieee80211_dev, &bssid);

		switch (key_conf->suite) {
		case IEEE80211_SECURITY_SUITE_WEP40:
			key_type = AR9271_KEY_TABLE_TYPE_WEP40;
			break;
		case IEEE80211_SECURITY_SUITE_WEP104:
			key_type = AR9271_KEY_TABLE_TYPE_WEP104;
			break;
		case IEEE80211_SECURITY_SUITE_TKIP:
			key_type = AR9271_KEY_TABLE_TYPE_TKIP;
			break;
		case IEEE80211_SECURITY_SUITE_CCMP:
			key_type = AR9271_KEY_TABLE_TYPE_CCMP;
			break;
		default:
			key_type = -1;
		}

		uint8_t key_id =
		    (key_conf->flags & IEEE80211_KEY_FLAG_TYPE_PAIRWISE) ?
		    AR9271_STA_KEY_INDEX : key_conf->id;

		reg_ptr = AR9271_KEY_TABLE(key_id);
		mic_reg_ptr = AR9271_KEY_TABLE(key_id + 64);
		data_start = (void *) key_conf->data;

		key[0] = uint32_t_le2host(*((uint32_t *) data_start));
		key[1] = uint16_t_le2host(*((uint16_t *) (data_start + 4)));
		key[2] = uint32_t_le2host(*((uint32_t *) (data_start + 6)));
		key[3] = uint16_t_le2host(*((uint16_t *) (data_start + 10)));
		key[4] = uint32_t_le2host(*((uint32_t *) (data_start + 12)));

		if ((key_conf->suite == IEEE80211_SECURITY_SUITE_WEP40) ||
		    (key_conf->suite == IEEE80211_SECURITY_SUITE_WEP104))
			key[4] &= 0xFF;

		wmi_reg_write(ar9271->htc_device, reg_ptr + 0, key[0]);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 4, key[1]);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 8, key[2]);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 12, key[3]);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 16, key[4]);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 20, key_type);

		uint32_t macl;
		uint32_t mach;
		if (key_conf->flags & IEEE80211_KEY_FLAG_TYPE_PAIRWISE) {
			data_start = (void *) bssid.address;
			macl = uint32_t_le2host(*((uint32_t *) data_start));
			mach = uint16_t_le2host(*((uint16_t *) (data_start + 4)));
		} else {
			macl = 0;
			mach = 0;
		}

		macl >>= 1;
		macl |= (mach & 1) << 31;
		mach >>= 1;
		mach |= 0x8000;

		wmi_reg_write(ar9271->htc_device, reg_ptr + 24, macl);
		wmi_reg_write(ar9271->htc_device, reg_ptr + 28, mach);

		/* Setup MIC keys for TKIP. */
		if (key_conf->suite == IEEE80211_SECURITY_SUITE_TKIP) {
			uint32_t mic[5];
			uint8_t *gen_mic = data_start + IEEE80211_TKIP_RX_MIC_OFFSET;
			uint8_t *tx_mic;

			if (key_conf->flags & IEEE80211_KEY_FLAG_TYPE_GROUP)
				tx_mic = gen_mic;
			else
				tx_mic = data_start + IEEE80211_TKIP_TX_MIC_OFFSET;

			mic[0] = uint32_t_le2host(*((uint32_t *) gen_mic));
			mic[1] = uint16_t_le2host(*((uint16_t *) (tx_mic + 2))) & 0xFFFF;
			mic[2] = uint32_t_le2host(*((uint32_t *) (gen_mic + 4)));
			mic[3] = uint16_t_le2host(*((uint16_t *) tx_mic)) & 0xFFFF;
			mic[4] = uint32_t_le2host(*((uint32_t *) (tx_mic + 4)));

			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 0, mic[0]);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 4, mic[1]);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 8, mic[2]);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 12, mic[3]);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 16, mic[4]);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 20,
			    AR9271_KEY_TABLE_TYPE_CLR);

			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 24, 0);
			wmi_reg_write(ar9271->htc_device, mic_reg_ptr + 28, 0);
		}

		if (key_conf->flags & IEEE80211_KEY_FLAG_TYPE_GROUP)
			ieee80211_setup_key_confirm(ieee80211_dev, true);
	} else {
		/* TODO: Delete keys from device */
		ieee80211_setup_key_confirm(ieee80211_dev, false);
	}

	return EOK;
}

static errno_t ar9271_ieee80211_tx_handler(ieee80211_dev_t *ieee80211_dev,
    void *buffer, size_t buffer_size)
{
	assert(ieee80211_dev);

	size_t complete_size;
	size_t offset;
	void *complete_buffer;
	int endpoint;

	ar9271_t *ar9271 = (ar9271_t *) ieee80211_get_specific(ieee80211_dev);

	uint16_t frame_ctrl = *((uint16_t *) buffer);
	if (ieee80211_is_data_frame(frame_ctrl)) {
		offset = sizeof(htc_tx_data_header_t) +
		    sizeof(htc_frame_header_t);
		complete_size = buffer_size + offset;
		complete_buffer = malloc(complete_size);
		memset(complete_buffer, 0, complete_size);

		/*
		 * Because we handle just station mode yet, node ID and VIF ID
		 * are fixed.
		 */
		htc_tx_data_header_t *data_header =
		    (htc_tx_data_header_t *)
		    (complete_buffer + sizeof(htc_frame_header_t));
		data_header->data_type = HTC_DATA_NORMAL;
		data_header->node_idx = 1;
		data_header->vif_idx = 0;
		data_header->cookie = 0;

		if (ieee80211_query_using_key(ieee80211_dev)) {
			data_header->keyix = AR9271_STA_KEY_INDEX;

			int sec_suite =
			    ieee80211_get_pairwise_security(ieee80211_dev);

			switch (sec_suite) {
			case IEEE80211_SECURITY_SUITE_WEP40:
			case IEEE80211_SECURITY_SUITE_WEP104:
				data_header->key_type = AR9271_KEY_TYPE_WEP;
				break;
			case IEEE80211_SECURITY_SUITE_TKIP:
				data_header->key_type = AR9271_KEY_TYPE_TKIP;
				break;
			case IEEE80211_SECURITY_SUITE_CCMP:
				data_header->key_type = AR9271_KEY_TYPE_AES;
				break;
			}
		} else {
			data_header->key_type = 0;
			data_header->keyix = 0xFF;
		}

		endpoint = ar9271->htc_device->endpoints.data_be_endpoint;
	} else {
		offset = sizeof(htc_tx_management_header_t) +
		    sizeof(htc_frame_header_t);
		complete_size = buffer_size + offset;
		complete_buffer = malloc(complete_size);
		memset(complete_buffer, 0, complete_size);

		/*
		 * Because we handle just station mode yet, node ID and VIF ID
		 * are fixed.
		 */
		htc_tx_management_header_t *mgmt_header =
		    (htc_tx_management_header_t *)
		    (complete_buffer + sizeof(htc_frame_header_t));
		mgmt_header->node_idx = 0;
		mgmt_header->vif_idx = 0;
		mgmt_header->cookie = 0;
		mgmt_header->keyix = 0xFF;

		endpoint = ar9271->htc_device->endpoints.mgmt_endpoint;
	}

	/* Copy IEEE802.11 data to new allocated buffer with HTC headers. */
	memcpy(complete_buffer + offset, buffer, buffer_size);

	htc_send_data_message(ar9271->htc_device, complete_buffer,
	    complete_size, endpoint);

	free(complete_buffer);

	return EOK;
}

static errno_t ar9271_ieee80211_start(ieee80211_dev_t *ieee80211_dev)
{
	assert(ieee80211_dev);

	ar9271_t *ar9271 = (ar9271_t *) ieee80211_get_specific(ieee80211_dev);

	wmi_send_command(ar9271->htc_device, WMI_FLUSH_RECV, NULL, 0, NULL);

	errno_t rc = hw_reset(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to do HW reset.\n");
		return rc;
	}

	uint16_t htc_mode = host2uint16_t_be(1);
	wmi_send_command(ar9271->htc_device, WMI_SET_MODE,
	    (uint8_t *) &htc_mode, sizeof(htc_mode), NULL);
	wmi_send_command(ar9271->htc_device, WMI_ATH_INIT, NULL, 0, NULL);
	wmi_send_command(ar9271->htc_device, WMI_START_RECV, NULL, 0, NULL);
	wmi_send_command(ar9271->htc_device, WMI_ENABLE_INTR, NULL, 0, NULL);

	rc = hw_rx_init(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to initialize RX.\n");
		return rc;
	}

	/* Send capability message to target. */
	htc_cap_msg_t cap_msg;
	cap_msg.ampdu_limit = host2uint32_t_be(0xffff);
	cap_msg.ampdu_subframes = 0xff;
	cap_msg.enable_coex = 0;
	cap_msg.tx_chainmask = 0x1;

	wmi_send_command(ar9271->htc_device, WMI_TARGET_IC_UPDATE,
	    (uint8_t *) &cap_msg, sizeof(cap_msg), NULL);

	rc = htc_init_new_vif(ar9271->htc_device);
	if (rc != EOK) {
		usb_log_error("Failed to initialize new VIF.\n");
		return rc;
	}

	/* Add data polling fibril. */
	fid_t fibril = fibril_create(ar9271_data_polling, ar9271);
	if (fibril == 0)
		return ENOMEM;

	fibril_add_ready(fibril);

	ar9271->starting_up = false;
	ieee80211_set_ready(ieee80211_dev, true);

	usb_log_info("Device fully initialized.\n");

	return EOK;
}

static errno_t ar9271_init(ar9271_t *ar9271, usb_device_t *usb_device, const usb_endpoint_description_t **endpoints)
{
	ar9271->starting_up = true;
	ar9271->usb_device = usb_device;

	fibril_mutex_initialize(&ar9271->ar9271_lock);

	ar9271->ath_device = calloc(1, sizeof(ath_t));
	if (!ar9271->ath_device) {
		usb_log_error("Failed to allocate memory for ath device "
		    "structure.\n");
		return ENOMEM;
	}

	errno_t rc = ath_usb_init(ar9271->ath_device, usb_device, endpoints);
	if (rc != EOK) {
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize ath device.\n");
		return rc;
	}

	/* IEEE 802.11 framework structure initialization. */
	ar9271->ieee80211_dev = ieee80211_device_create();
	if (!ar9271->ieee80211_dev) {
		free(ar9271->ath_device);
		usb_log_error("Failed to allocate memory for IEEE80211 device "
		    "structure.\n");
		return ENOMEM;
	}

	rc = ieee80211_device_init(ar9271->ieee80211_dev, ar9271->ddf_dev);
	if (rc != EOK) {
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize IEEE80211 device structure."
		    "\n");
		return rc;
	}

	ieee80211_set_specific(ar9271->ieee80211_dev, ar9271);

	/* HTC device structure initialization. */
	ar9271->htc_device = calloc(1, sizeof(htc_device_t));
	if (!ar9271->htc_device) {
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to allocate memory for HTC device "
		    "structure.\n");
		return ENOMEM;
	}

	rc = htc_device_init(ar9271->ath_device, ar9271->ieee80211_dev,
	    ar9271->htc_device);
	if (rc != EOK) {
		free(ar9271->htc_device);
		free(ar9271->ieee80211_dev);
		free(ar9271->ath_device);
		usb_log_error("Failed to initialize HTC device structure.\n");
		return rc;
	}

	return EOK;
}

/** Upload firmware to WiFi device.
 *
 * @param ar9271 AR9271 device structure
 *
 * @return EOK if succeed, error code otherwise
 *
 */
static errno_t ar9271_upload_fw(ar9271_t *ar9271)
{
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
	if (fw_data == NULL) {
		fclose(fw_file);
		usb_log_error("Failed allocating memory for firmware.\n");
		return ENOMEM;
	}

	fread(fw_data, file_size, 1, fw_file);
	fclose(fw_file);

	/* Upload FW to device. */
	uint64_t remain_size = file_size;
	uint32_t current_addr = AR9271_FW_ADDRESS;
	uint8_t *current_data = fw_data;
	uint8_t *buffer = malloc(MAX_TRANSFER_SIZE);

	while (remain_size > 0) {
		size_t chunk_size = min(remain_size, MAX_TRANSFER_SIZE);
		memcpy(buffer, current_data, chunk_size);
		usb_pipe_t *ctrl_pipe = usb_device_get_default_pipe(usb_device);
		errno_t rc = usb_control_request_set(ctrl_pipe,
		    USB_REQUEST_TYPE_VENDOR,
		    USB_REQUEST_RECIPIENT_DEVICE,
		    AR9271_FW_DOWNLOAD,
		    uint16_host2usb(current_addr >> 8),
		    0, buffer, chunk_size);
		if (rc != EOK) {
			free(fw_data);
			free(buffer);
			usb_log_error("Error while uploading firmware. "
			    "Error: %s\n", str_error_name(rc));
			return rc;
		}

		remain_size -= chunk_size;
		current_addr += chunk_size;
		current_data += chunk_size;
	}

	free(fw_data);
	free(buffer);

	/*
	 * Send command that firmware is successfully uploaded.
	 * This should initiate creating confirmation message in
	 * device side buffer which we will check in htc_check_ready function.
	*/
	usb_pipe_t *ctrl_pipe = usb_device_get_default_pipe(usb_device);
	errno_t rc = usb_control_request_set(ctrl_pipe,
	    USB_REQUEST_TYPE_VENDOR,
	    USB_REQUEST_RECIPIENT_DEVICE,
	    AR9271_FW_DOWNLOAD_COMP,
	    uint16_host2usb(AR9271_FW_OFFSET >> 8),
	    0, NULL, 0);

	if (rc != EOK) {
		usb_log_error("IO error when sending fw upload confirmation "
		    "message.\n");
		return rc;
	}

	usb_log_info("Firmware uploaded successfully.\n");

	/* Wait until firmware is ready - wait for 1 second to be sure. */
	async_sleep(1);

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
	const char *err_msg = NULL;
	errno_t rc = usb_device_create_ddf(dev, endpoints, &err_msg);
	if (rc != EOK) {
		usb_log_error("Failed to create USB device: %s, "
		    "ERR_NUM = %s\n", err_msg, str_error_name(rc));
		return NULL;
	}

	/* AR9271 structure initialization. */
	ar9271_t *ar9271 = calloc(1, sizeof(ar9271_t));
	if (!ar9271) {
		usb_log_error("Failed to allocate memory for device "
		    "structure.\n");
		return NULL;
	}

	ar9271->ddf_dev = dev;

	rc = ar9271_init(ar9271, usb_device_get(dev), endpoints);
	if (rc != EOK) {
		free(ar9271);
		usb_log_error("Failed to initialize AR9271 structure: %s\n",
		    str_error_name(rc));
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
 * @return EOK if succeed, error code otherwise
 */
static errno_t ar9271_add_device(ddf_dev_t *dev)
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
	errno_t rc = htc_init(ar9271->htc_device);
	if (rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("HTC initialization failed.\n");
		return rc;
	}

	/* Initialize AR9271 HW. */
	rc = hw_init(ar9271);
	if (rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("HW initialization failed.\n");
		return rc;
	}

	/* Initialize AR9271 IEEE 802.11 framework. */
	rc = ieee80211_init(ar9271->ieee80211_dev, &ar9271_ieee80211_ops,
	    &ar9271_ieee80211_iface, &ar9271_ieee80211_nic_iface,
	    &ar9271_ieee80211_dev_ops);
	if (rc != EOK) {
		ar9271_delete_dev_data(ar9271);
		usb_log_error("Failed to initialize IEEE80211 framework.\n");
		return rc;
	}

	nic_set_filtering_change_handlers(nic_get_from_ddf_dev(dev),
	    ar9271_on_unicast_mode_change, ar9271_on_multicast_mode_change,
	    ar9271_on_broadcast_mode_change, NULL, NULL);

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
