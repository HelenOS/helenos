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

/** @addtogroup libnet
 *  @{
 */

/** @file ieee80211.c
 * 
 * IEEE 802.11 interface implementation.
 */

#include <errno.h>
#include <byteorder.h>

#include <ieee80211_impl.h>
#include <ieee80211.h>

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

/** Network interface options for IEEE 802.11 driver. */
static nic_iface_t ieee80211_nic_iface;

/** Basic driver operations for IEEE 802.11 NIC driver. */
static driver_ops_t ieee80211_nic_driver_ops;

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

/** Basic NIC device operations for IEEE802.11 driver. */
static ddf_dev_ops_t ieee80211_nic_dev_ops = {
	.open = ieee80211_open
};

static int ieee80211_set_operations(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops)
{
	/* IEEE802.11 start operation must be implemented. */
	if(!ieee80211_ops->start)
		return EINVAL;
	
	/* IEEE802.11 TX handler must be implemented. */
	if(!ieee80211_ops->tx_handler)
		return EINVAL;
	
	/* IEEE802.11 set frequency handler must be implemented. */
	if(!ieee80211_ops->set_freq)
		return EINVAL;
	
	if(!ieee80211_ops->scan)
		ieee80211_ops->scan = ieee80211_scan_impl;
	
	ieee80211_dev->ops = ieee80211_ops;
	
	return EOK;
}

/**
 * Initialize an IEEE802.11 framework structure.
 * 
 * @param ieee80211_dev Device structure to initialize.
 * @param ieee80211_ops Structure with implemented IEEE802.11 operations.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, void *driver_data,
	ddf_dev_t *ddf_dev)
{
	ieee80211_dev->ddf_dev = ddf_dev;
	ieee80211_dev->driver_data = driver_data;
	ieee80211_dev->started = false;
	ieee80211_dev->current_op_mode = IEEE80211_OPMODE_STATION;
	
	memcpy(ieee80211_dev->bssid_mask, ieee80211_broadcast_mac_addr, 
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
 * @param ieee80211_ops Structure with implemented IEEE802.11 operations.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int ieee80211_init(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops)
{
	int rc = ieee80211_set_operations(ieee80211_dev, ieee80211_ops);
	if(rc != EOK)
		return rc;
	
	nic_driver_implement(&ieee80211_nic_driver_ops, &ieee80211_nic_dev_ops,
	    &ieee80211_nic_iface);
	
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	
	/** TODO: Set NIC handlers here. */
	
	ddf_fun_t *fun = ddf_fun_create(ieee80211_dev->ddf_dev, fun_exposed, 
		"port0");
	if (fun == NULL) {
		return EINVAL;
	}
	
	nic_set_ddf_fun(nic, fun);
	ddf_fun_set_ops(fun, &ieee80211_nic_dev_ops);
	
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
	
	return EOK;
}

static uint8_t ieee80211_freq_to_channel(uint16_t freq)
{
	return (freq - IEEE80211_FIRST_FREQ) / IEEE80211_CHANNEL_GAP + 1;
}

int ieee80211_probe_request(ieee80211_dev_t *ieee80211_dev)
{
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);
	
	size_t data_rates_size = 
		sizeof(ieee80211bg_data_rates) / 
		sizeof(ieee80211bg_data_rates[0]);
	
	size_t ext_data_rates_size = 
		sizeof(ieee80211bg_ext_data_rates) / 
		sizeof(ieee80211bg_ext_data_rates[0]);
	
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
	uint8_t *it = buffer + sizeof(ieee80211_mgmt_header_t) + 2;
	
	*it++ = IEEE80211_RATES_IE;
	*it++ = data_rates_size;
	memcpy(it, ieee80211bg_data_rates, data_rates_size);
	it += data_rates_size;
	
	*it++ = IEEE80211_EXT_RATES_IE;
	*it++ = ext_data_rates_size;
	memcpy(it, ieee80211bg_ext_data_rates, ext_data_rates_size);
	it += ext_data_rates_size;
	
	*it++ = IEEE80211_CHANNEL_IE;
	*it++ = 1;
	*it = ieee80211_freq_to_channel(ieee80211_dev->current_freq);
	
	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);
	
	free(buffer);
	
	return EOK;
}

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

/** @}
 */
