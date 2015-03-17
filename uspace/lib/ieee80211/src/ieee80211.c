/*
 * Copyright (c) 2015 Jan Kolarik
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

/** @addtogroup libieee80211
 * @{
 */

/** @file ieee80211.c
 * 
 * IEEE 802.11 interface implementation.
 */

#include <macros.h>
#include <errno.h>

#include <ieee80211.h>
#include <ieee80211_impl.h>
#include <ieee80211_iface_impl.h>
#include <ieee80211_private.h>
#include <ops/ieee80211.h>

/** Broadcast MAC address used to spread probe request through channel. */
static const uint8_t ieee80211_broadcast_mac_addr[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

/** IEEE 802.11 b/g supported data rates in units of 500 kb/s. */
static const uint8_t ieee80211bg_data_rates[] = {
	2, 4, 11, 12, 18, 22, 24, 36
};

/** IEEE 802.11 b/g extended supported data rates in units of 500 kb/s.
 * 
 *  These are defined separately, because probe request message can
 *  only handle up to 8 data rates in supported rates IE. 
 */
static const uint8_t ieee80211bg_ext_data_rates[] = {
	48, 72, 96, 108
};

inline bool ieee80211_is_data_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);
	
	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_TYPE)
		== IEEE80211_DATA_FRAME;
}

inline bool ieee80211_is_mgmt_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);
	
	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_TYPE)
		== IEEE80211_MGMT_FRAME;
}

inline bool ieee80211_is_beacon_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);
	
	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE)
		== IEEE80211_MGMT_BEACON_FRAME;
}

inline bool ieee80211_is_probe_response_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);
	
	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE)
		== IEEE80211_MGMT_PROBE_RESP_FRAME;
}

/**
 * Get driver-specific structure for IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * 
 * @return Driver-specific structure.
 */
void *ieee80211_get_specific(ieee80211_dev_t* ieee80211_dev)
{
	return ieee80211_dev->specific;
}

/**
 * Set driver-specific structure for IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * @param specific Driver-specific structure.
 */
void ieee80211_set_specific(ieee80211_dev_t* ieee80211_dev,
	void *specific)
{
	ieee80211_dev->specific = specific;
}

/**
 * Get related DDF device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * 
 * @return DDF device.
 */
ddf_dev_t *ieee80211_get_ddf_dev(ieee80211_dev_t* ieee80211_dev)
{
	return ieee80211_dev->ddf_dev;
}

/**
 * Query current operating mode of IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * 
 * @return Current IEEE 802.11 operating mode.
 */
ieee80211_operating_mode_t ieee80211_query_current_op_mode(ieee80211_dev_t* ieee80211_dev)
{
	return ieee80211_dev->current_op_mode;
}

/**
 * Query current frequency of IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * 
 * @return Current device operating frequency.
 */
uint16_t ieee80211_query_current_freq(ieee80211_dev_t* ieee80211_dev)
{
	return ieee80211_dev->current_freq;
}

/**
 * Report current operating mode for IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * @param op_mode Current IEEE 802.11 operating mode.
 */
void ieee80211_report_current_op_mode(ieee80211_dev_t* ieee80211_dev,
	ieee80211_operating_mode_t op_mode)
{
	ieee80211_dev->current_op_mode = op_mode;
}

/**
 * Report current frequency for IEEE 802.11 device.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * @param freq Current device operating frequency.
 */
void ieee80211_report_current_freq(ieee80211_dev_t* ieee80211_dev,
	uint16_t freq)
{
	ieee80211_dev->current_freq = freq;
}

/**
 * Implementation of NIC open callback for IEEE 802.11 devices.
 * 
 * @param fun NIC function.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int ieee80211_open(ddf_fun_t *fun)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	ieee80211_dev_t *ieee80211_dev = nic_get_specific(nic_data);
	
	if(ieee80211_dev->started) {
		return EOK;
	} else {
		ieee80211_dev->started = true;
	}
	
	int rc = ieee80211_dev->ops->start(ieee80211_dev);
	if(rc != EOK)
		return rc;
	
	rc = ieee80211_dev->ops->scan(ieee80211_dev);
	if(rc != EOK)
		return rc;
	
	return EOK;
}

/** 
 * Send frame handler.
 */
static void ieee80211_send_frame(nic_t *nic, void *data, size_t size)
{
	ieee80211_dev_t *ieee80211_dev = (ieee80211_dev_t *) 
		nic_get_specific(nic);
	
	ieee80211_dev->ops->tx_handler(ieee80211_dev, data, size);
}

/**
 * Fill out IEEE 802.11 device functions implementations.
 * 
 * @param ieee80211_dev IEEE 802.11 device.
 * @param ieee80211_ops Callbacks implementation.
 * @param ieee80211_iface Interface functions implementation.
 * @param nic_iface NIC interface functions implementation.
 * @param nic_dev_ops NIC device functions implementation.
 * 
 * @return EINVAL when missing pointer to ieee80211_ops or ieee80211_iface,
 * otherwise EOK.
 */
static int ieee80211_implement(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops, ieee80211_iface_t *ieee80211_iface,
	nic_iface_t *nic_iface, ddf_dev_ops_t *nic_dev_ops)
{
	if(ieee80211_ops) {
		if(!ieee80211_ops->start)
			ieee80211_ops->start = ieee80211_start_impl;

		if(!ieee80211_ops->tx_handler)
			ieee80211_ops->tx_handler = ieee80211_tx_handler_impl;

		if(!ieee80211_ops->set_freq)
			ieee80211_ops->set_freq = ieee80211_set_freq_impl;
		
		if(!ieee80211_ops->scan)
			ieee80211_ops->scan = ieee80211_scan_impl;
	} else {
		return EINVAL;
	}
	
	ieee80211_dev->ops = ieee80211_ops;
	
	if(ieee80211_iface) {
		if(nic_dev_ops)
			if (!nic_dev_ops->interfaces[IEEE80211_DEV_IFACE])
				nic_dev_ops->interfaces[IEEE80211_DEV_IFACE] = ieee80211_iface;
		
		if(!ieee80211_iface->get_scan_results)
			ieee80211_iface->get_scan_results = ieee80211_get_scan_results_impl;
		
		if(!ieee80211_iface->connect)
			ieee80211_iface->connect = ieee80211_connect_impl;
	} else {
		return EINVAL;
	}
	
	if(nic_dev_ops) {
		if(!nic_dev_ops->open)
			nic_dev_ops->open = ieee80211_open;
	} else {
		return EINVAL;
	}
	
	ieee80211_dev->iface = ieee80211_iface;
	
	nic_driver_implement(NULL, nic_dev_ops, nic_iface);
	
	return EOK;
}

/**
 * Allocate IEEE802.11 device structure.
 * 
 * @return Pointer to allocated IEEE802.11 device structure.
 */
ieee80211_dev_t *ieee80211_device_create()
{
	return calloc(1, sizeof(ieee80211_dev_t));
}

/**
 * Initialize an IEEE802.11 framework structure.
 * 
 * @param ieee80211_dev Device structure to initialize.
 * @param ddf_dev Pointer to backing DDF device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, ddf_dev_t *ddf_dev)
{
	ieee80211_dev->ddf_dev = ddf_dev;
	ieee80211_dev->started = false;
	ieee80211_dev->current_op_mode = IEEE80211_OPMODE_STATION;
	memcpy(ieee80211_dev->bssid_mask.address, ieee80211_broadcast_mac_addr, 
		ETH_ADDR);
	
	/* Bind NIC to device */
	nic_t *nic = nic_create_and_bind(ddf_dev);
	if (!nic) {
		return ENOMEM;
	}
	
	nic_set_specific(nic, ieee80211_dev);
	
	return EOK;
}

/**
 * IEEE802.11 WiFi framework initialization.
 * 
 * @param ieee80211_dev Device structure to initialize.
 * @param ieee80211_ops Structure with implemented IEEE802.11 device operations.
 * @param ieee80211_iface Structure with implemented IEEE802.11 interface 
 * operations.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_init(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops, ieee80211_iface_t *ieee80211_iface,
	nic_iface_t *ieee80211_nic_iface, ddf_dev_ops_t *ieee80211_nic_dev_ops)
{
	int rc = ieee80211_implement(ieee80211_dev, 
		ieee80211_ops, ieee80211_iface,
		ieee80211_nic_iface, ieee80211_nic_dev_ops);
	if(rc != EOK) {
		return rc;
	}
	
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	
	/** TODO: Set NIC handlers here. */
	nic_set_send_frame_handler(nic, ieee80211_send_frame);
	
	ddf_fun_t *fun = ddf_fun_create(ieee80211_dev->ddf_dev, fun_exposed, 
		"port0");
	if (fun == NULL) {
		return EINVAL;
	}
	
	nic_set_ddf_fun(nic, fun);
	ddf_fun_set_ops(fun, ieee80211_nic_dev_ops);
	
	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_fun_destroy(fun);
		return rc;
	}
	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_NIC);
	if (rc != EOK) {
		ddf_fun_unbind(fun);
		return rc;
	}
	rc = ddf_fun_add_to_category(fun, DEVICE_CATEGORY_IEEE80211);
	if (rc != EOK) {
		ddf_fun_unbind(fun);
		return rc;
	}
	
	return EOK;
}

static uint8_t ieee80211_freq_to_channel(uint16_t freq)
{
	return (freq - IEEE80211_FIRST_FREQ) / IEEE80211_CHANNEL_GAP + 1;
}

/**
 * Probe request implementation.
 * 
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_probe_request(ieee80211_dev_t *ieee80211_dev)
{
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);
	
	size_t data_rates_size = ARRAY_SIZE(ieee80211bg_data_rates);
	size_t ext_data_rates_size = ARRAY_SIZE(ieee80211bg_ext_data_rates);
	
	/* 3 headers - (rates, ext rates, current channel) and their data
	 * lengths + pad. 
	 */
	size_t payload_size = 
		sizeof(ieee80211_ie_header_t) * 3 +
		data_rates_size + ext_data_rates_size + sizeof(uint8_t) + 2;
	
	size_t buffer_size = sizeof(ieee80211_mgmt_header_t) + payload_size;
	void *buffer = malloc(buffer_size);
	memset(buffer, 0, buffer_size);
	
	ieee80211_mgmt_header_t *mgmt_header = 
		(ieee80211_mgmt_header_t *) buffer;
	
	mgmt_header->frame_ctrl = host2uint16_t_le(
		IEEE80211_MGMT_FRAME | 
		IEEE80211_MGMT_PROBE_REQ_FRAME
		);
	memcpy(mgmt_header->dest_addr, ieee80211_broadcast_mac_addr, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, ieee80211_broadcast_mac_addr, ETH_ADDR);
	
	/* Jump to payload -> header + padding. */
	ieee80211_ie_header_t *rates_ie_header = (ieee80211_ie_header_t *) 
		((void *)buffer + sizeof(ieee80211_mgmt_header_t) + 2);
	rates_ie_header->element_id = IEEE80211_RATES_IE;
	rates_ie_header->length = data_rates_size;
	memcpy(rates_ie_header + sizeof(ieee80211_ie_header_t), 
		ieee80211bg_data_rates, 
		data_rates_size);
	
	ieee80211_ie_header_t *ext_rates_ie_header = (ieee80211_ie_header_t *) 
		((void *)rates_ie_header + sizeof(ieee80211_ie_header_t) + 
		data_rates_size);
	ext_rates_ie_header->element_id = IEEE80211_EXT_RATES_IE;
	ext_rates_ie_header->length = ext_data_rates_size;
	memcpy(ext_rates_ie_header + sizeof(ieee80211_ie_header_t), 
		ieee80211bg_ext_data_rates, 
		ext_data_rates_size);
	
	ieee80211_ie_header_t *chan_ie_header = (ieee80211_ie_header_t *) 
		((void *)ext_rates_ie_header + sizeof(ieee80211_ie_header_t) + 
		ext_data_rates_size);
	chan_ie_header->element_id = IEEE80211_CHANNEL_IE;
	chan_ie_header->length = 1;
	uint8_t *it = (uint8_t *) ((void *)chan_ie_header + 
		sizeof(ieee80211_ie_header_t));
	*it = ieee80211_freq_to_channel(ieee80211_dev->current_freq);
	
	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);
	
	free(buffer);
	
	return EOK;
}

/**
 * Probe authentication implementation.
 * 
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_probe_auth(ieee80211_dev_t *ieee80211_dev)
{
	uint8_t test_bssid[] = {0x14, 0xF6, 0x5A, 0xAF, 0x5E, 0xB7};
	
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);
	
	size_t buffer_size = sizeof(ieee80211_mgmt_header_t) + 
		sizeof(ieee80211_auth_body_t);
	void *buffer = malloc(buffer_size);
	memset(buffer, 0, buffer_size);
	
	ieee80211_mgmt_header_t *mgmt_header = 
		(ieee80211_mgmt_header_t *) buffer;
	
	mgmt_header->frame_ctrl = host2uint16_t_le(
		IEEE80211_MGMT_FRAME | 
		IEEE80211_MGMT_AUTH_FRAME
		);
	memcpy(mgmt_header->dest_addr, test_bssid, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, test_bssid, ETH_ADDR);
	
	ieee80211_auth_body_t *auth_body =
		(ieee80211_auth_body_t *) 
		(buffer + sizeof(ieee80211_mgmt_header_t));
	auth_body->auth_alg = host2uint16_t_le(0);
	auth_body->auth_trans_no = host2uint16_t_le(0);
	
	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);
	
	free(buffer);
	
	return EOK;
}

/**
 * Process probe response and store results.
 * 
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int ieee80211_process_probe_response(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_mgmt_header_t *mgmt_header)
{
	ieee80211_ie_header_t *ssid_ie_header = (ieee80211_ie_header_t *) 
		((void *)mgmt_header + sizeof(ieee80211_mgmt_header_t) + 
		sizeof(ieee80211_beacon_start_t));
	
	if(ssid_ie_header->length > 0) {
		uint8_t *results_length = &ieee80211_dev->ap_list.length;
		
		ieee80211_scan_result_t *ap_data = 
			&ieee80211_dev->ap_list.results[(*results_length)++];
		
		memset(ap_data, 0, sizeof(ieee80211_scan_result_t));
		
		uint8_t *ssid_start = (uint8_t *) 
			((void *)ssid_ie_header + 
			sizeof(ieee80211_ie_header_t));

		memcpy(ap_data->bssid.address, mgmt_header->bssid, ETH_ADDR);
		memcpy(ap_data->ssid, ssid_start, ssid_ie_header->length);
		ap_data->ssid[ssid_ie_header->length] = '\0';
	}
	
	return EOK;
}

/**
 * IEEE 802.11 RX frames handler.
 * 
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param buffer Buffer with data.
 * @param buffer_size Size of buffer.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_rx_handler(ieee80211_dev_t *ieee80211_dev, void *buffer,
	size_t buffer_size)
{
	uint16_t frame_ctrl = *((uint16_t *) buffer);
	if(ieee80211_is_mgmt_frame(frame_ctrl)) {
		ieee80211_mgmt_header_t *mgmt_header =
			(ieee80211_mgmt_header_t *) buffer;
		
		if(ieee80211_is_probe_response_frame(mgmt_header->frame_ctrl) ||
			ieee80211_is_beacon_frame(mgmt_header->frame_ctrl)) {
			return ieee80211_process_probe_response(ieee80211_dev,
				mgmt_header);
		}
		// TODO
	} else if(ieee80211_is_data_frame(frame_ctrl)) {
		nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
		size_t frame_size = buffer_size - sizeof(ieee80211_data_header_t);
		nic_frame_t *frame = nic_alloc_frame(nic, frame_size); 
		if (frame != NULL) {
			memcpy(frame->data, 
				buffer + sizeof(ieee80211_data_header_t), 
				frame_size);
			nic_received_frame(nic, frame);
		}
	} else {
		// TODO
	}
	
	return EOK;
}

/** @}
 */
