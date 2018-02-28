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

#include <stdio.h>
#include <crypto.h>
#include <str.h>
#include <macros.h>
#include <errno.h>
#include <ieee80211.h>
#include <ieee80211_impl.h>
#include <ieee80211_iface_impl.h>
#include <ieee80211_private.h>
#include <ops/ieee80211.h>

#define IEEE80211_DATA_RATES_SIZE      8
#define IEEE80211_EXT_DATA_RATES_SIZE  4

#define ATOMIC_GET(state)

/** Frame encapsulation used in IEEE 802.11. */
static const uint8_t rfc1042_header[] = {
	0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00
};

/** Broadcast MAC address. */
static const uint8_t ieee80211_broadcast_mac_addr[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/** Check data frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is data frame, otherwise false.
 *
 */
inline bool ieee80211_is_data_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_TYPE) ==
	    IEEE80211_DATA_FRAME;
}

/** Check management frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is management frame, otherwise false.
 *
 */
inline bool ieee80211_is_mgmt_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_TYPE) ==
	    IEEE80211_MGMT_FRAME;
}

/** Check management beacon frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is beacon frame, otherwise false.
 *
 */
inline bool ieee80211_is_beacon_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE) ==
	    IEEE80211_MGMT_BEACON_FRAME;
}

/** Check management probe response frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is probe resp frame, otherwise false.
 *
 */
inline bool ieee80211_is_probe_response_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE) ==
	    IEEE80211_MGMT_PROBE_RESP_FRAME;
}

/** Check management authentication frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is auth frame, otherwise false.
 *
 */
inline bool ieee80211_is_auth_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE) ==
	    IEEE80211_MGMT_AUTH_FRAME;
}

/** Check management association response frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is assoc resp frame, otherwise false.
 *
 */
inline bool ieee80211_is_assoc_response_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FRAME_SUBTYPE) ==
	    IEEE80211_MGMT_ASSOC_RESP_FRAME;
}

/** Check data frame "to distribution system" direction.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is TODS frame, otherwise false.
 *
 */
inline bool ieee80211_is_tods_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_TODS);
}

/** Check data frame "from distribution system" direction.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it is FROMDS frame, otherwise false.
 *
 */
inline bool ieee80211_is_fromds_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_FROMDS);
}

/** Check if it is data frame containing payload data.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if it has payload data, otherwise false.
 *
 */
static inline bool ieee80211_has_data_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & (IEEE80211_FRAME_CTRL_FRAME_TYPE | 0x40)) ==
	    IEEE80211_DATA_FRAME;
}

/** Check if it is encrypted frame.
 *
 * @param frame_ctrl Frame control field in little endian (!).
 *
 * @return True if the frame is encrypted, otherwise false.
 *
 */
static inline bool ieee80211_is_encrypted_frame(uint16_t frame_ctrl)
{
	frame_ctrl = uint16_t_le2host(frame_ctrl);

	return (frame_ctrl & IEEE80211_FRAME_CTRL_PROTECTED);
}

/** Check if PAE packet is EAPOL-Key frame.
 *
 * @param key_frame Pointer to start of EAPOL frame.
 *
 * @return True if it is EAPOL-Key frame, otherwise false.
 *
 */
static inline bool
    ieee80211_is_eapol_key_frame(ieee80211_eapol_key_frame_t *key_frame)
{
	return (key_frame->packet_type == IEEE80211_EAPOL_KEY);
}

/** Generate packet sequence number.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return True if it has payload data, otherwise false.
 *
 */
static uint16_t ieee80211_get_sequence_number(ieee80211_dev_t *ieee80211_dev)
{
	uint16_t ret_val = ieee80211_dev->sequence_number;
	ieee80211_dev->sequence_number += (1 << 4);

	return ret_val;
}

/** Get driver-specific structure for IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return Driver-specific structure.
 *
 */
void *ieee80211_get_specific(ieee80211_dev_t *ieee80211_dev)
{
	return ieee80211_dev->specific;
}

/** Set driver-specific structure for IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 * @param specific      Driver-specific structure.
 *
 */
void ieee80211_set_specific(ieee80211_dev_t *ieee80211_dev,
    void *specific)
{
	ieee80211_dev->specific = specific;
}

/** Get related DDF device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return DDF device.
 *
 */
ddf_dev_t *ieee80211_get_ddf_dev(ieee80211_dev_t *ieee80211_dev)
{
	return ieee80211_dev->ddf_dev;
}

/** Query current operating mode of IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return Current IEEE 802.11 operating mode.
 *
 */
ieee80211_operating_mode_t
    ieee80211_query_current_op_mode(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_operating_mode_t op_mode = ieee80211_dev->current_op_mode;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return op_mode;
}

/** Query current frequency of IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return Current device operating frequency.
 *
 */
uint16_t ieee80211_query_current_freq(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	uint16_t current_freq = ieee80211_dev->current_freq;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return current_freq;
}

/** Query BSSID the device is connected to.
 *
 * Note: Expecting locked results_mutex.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 * @param bssid         Pointer to structure where should be stored BSSID.
 *
 */
void ieee80211_query_bssid(ieee80211_dev_t *ieee80211_dev,
    nic_address_t *bssid)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);

	if (bssid) {
		ieee80211_scan_result_link_t *res_link =
		    ieee80211_dev->bssid_info.res_link;

		if (res_link) {
			memcpy(bssid, &res_link->scan_result.bssid,
			    sizeof(nic_address_t));
		} else {
			nic_address_t broadcast_addr;
			memcpy(broadcast_addr.address,
			    ieee80211_broadcast_mac_addr, ETH_ADDR);
			memcpy(bssid, &broadcast_addr, sizeof(nic_address_t));
		}
	}

	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

/** Get AID of network we are connected to.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return AID.
 *
 */
uint16_t ieee80211_get_aid(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	uint16_t aid = ieee80211_dev->bssid_info.aid;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return aid;
}

/** Get pairwise security suite used for HW encryption.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return Security suite indicator.
 *
 */
int ieee80211_get_pairwise_security(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_scan_result_link_t *auth_link =
	    ieee80211_dev->bssid_info.res_link;
	int suite = auth_link->scan_result.security.pair_alg;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return suite;
}

/** Check if IEEE 802.11 device is connected to network.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return True if device is connected to network, otherwise false.
 *
 */
bool ieee80211_is_connected(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	bool conn_state =
	    ieee80211_dev->current_auth_phase == IEEE80211_AUTH_CONNECTED;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return conn_state;
}

void ieee80211_set_auth_phase(ieee80211_dev_t *ieee80211_dev,
    ieee80211_auth_phase_t auth_phase)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->current_auth_phase = auth_phase;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

ieee80211_auth_phase_t ieee80211_get_auth_phase(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_auth_phase_t conn_state = ieee80211_dev->current_auth_phase;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return conn_state;
}

void ieee80211_set_connect_request(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->pending_conn_req = true;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

bool ieee80211_pending_connect_request(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	bool conn_request = ieee80211_dev->pending_conn_req;
	ieee80211_dev->pending_conn_req = false;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return conn_request;
}

/** Report current operating mode for IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 * @param op_mode       Current IEEE 802.11 operating mode.
 *
 */
void ieee80211_report_current_op_mode(ieee80211_dev_t *ieee80211_dev,
    ieee80211_operating_mode_t op_mode)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->current_op_mode = op_mode;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

/** Report current frequency for IEEE 802.11 device.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 * @param freq          Current device operating frequency.
 *
 */
void ieee80211_report_current_freq(ieee80211_dev_t *ieee80211_dev,
    uint16_t freq)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->current_freq = freq;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

/** Check if IEEE 802.11 device is ready (fully initialized).
 *
 * @param ieee80211_dev IEEE 802.11 device.
 *
 * @return True if device is ready to work, otherwise false.
 *
 */
bool ieee80211_is_ready(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	bool ready_state = ieee80211_dev->ready;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return ready_state;
}

/** Set IEEE 802.11 device to ready state.
 *
 * @param ieee80211_dev IEEE 802.11 device.
 * @param ready         Ready state to be set.
 *
 */
void ieee80211_set_ready(ieee80211_dev_t *ieee80211_dev, bool ready)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->ready = ready;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

extern bool ieee80211_query_using_key(ieee80211_dev_t *ieee80211_dev)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	bool using_key = ieee80211_dev->using_hw_key;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return using_key;
}

void ieee80211_setup_key_confirm(ieee80211_dev_t *ieee80211_dev,
    bool using_key)
{
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	ieee80211_dev->using_hw_key = using_key;
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
}

static errno_t ieee80211_scan(void *arg)
{
	assert(arg);

	ieee80211_dev_t *ieee80211_dev = (ieee80211_dev_t *) arg;

	while (true) {
		ieee80211_dev->ops->scan(ieee80211_dev);
		async_usleep(SCAN_PERIOD_USEC);
	}

	return EOK;
}

/** Implementation of NIC open callback for IEEE 802.11 devices.
 *
 * @param fun NIC function.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ieee80211_open(ddf_fun_t *fun)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	ieee80211_dev_t *ieee80211_dev = nic_get_specific(nic_data);

	if (ieee80211_dev->started)
		return EOK;

	ieee80211_dev->started = true;

	errno_t rc = ieee80211_dev->ops->start(ieee80211_dev);
	if (rc != EOK)
		return rc;

	/* Add scanning fibril. */
	fid_t fibril = fibril_create(ieee80211_scan, ieee80211_dev);
	if (fibril == 0)
		return ENOMEM;

	fibril_add_ready(fibril);

	return EOK;
}

/** Send frame handler.
 *
 * @param nic  Pointer to NIC device.
 * @param data Data buffer.
 * @param size Data buffer size.
 *
 */
static void ieee80211_send_frame(nic_t *nic, void *data, size_t size)
{
	ieee80211_dev_t *ieee80211_dev = (ieee80211_dev_t *)
	    nic_get_specific(nic);

	ieee80211_auth_phase_t auth_phase =
	    ieee80211_get_auth_phase(ieee80211_dev);
	if ((auth_phase != IEEE80211_AUTH_ASSOCIATED) &&
	    (auth_phase != IEEE80211_AUTH_CONNECTED))
		return;

	ieee80211_scan_result_t *auth_data =
	    &ieee80211_dev->bssid_info.res_link->scan_result;

	/* We drop part of IEEE 802.3 ethernet header. */
	size_t drop_bytes = sizeof(eth_header_t) - 2;

	size_t complete_size = (size - drop_bytes) +
	    sizeof(ieee80211_data_header_t) +
	    ARRAY_SIZE(rfc1042_header);

	/* Init crypto data. */
	bool add_mic = false;
	size_t head_space = 0, mic_space = 0;
	uint16_t crypto = 0;
	uint8_t head_data[head_space];
	memset(head_data, 0, head_space);

	// TODO: Distinguish used key (pair/group) by dest address ?
	if (ieee80211_query_using_key(ieee80211_dev)) {
		int sec_suite = auth_data->security.pair_alg;
		switch (sec_suite) {
		case IEEE80211_SECURITY_SUITE_TKIP:
			head_space = IEEE80211_TKIP_HEADER_LENGTH;
			mic_space = MIC_LENGTH;
			add_mic = true;
			break;
		case IEEE80211_SECURITY_SUITE_CCMP:
			head_space = IEEE80211_CCMP_HEADER_LENGTH;
			head_data[3] = 0x20;
			break;
		default:
			break;
		}

		crypto = uint16_t_le2host(IEEE80211_FRAME_CTRL_PROTECTED);
	}

	complete_size += head_space + mic_space;

	void *complete_buffer = malloc(complete_size);
	if (!complete_buffer)
		return;

	memset(complete_buffer, 0, complete_size);

	if (head_space)
		memcpy(complete_buffer + sizeof(ieee80211_data_header_t),
		    head_data, head_space);

	memcpy(complete_buffer + sizeof(ieee80211_data_header_t) + head_space,
	    rfc1042_header, ARRAY_SIZE(rfc1042_header));

	memcpy(complete_buffer + sizeof(ieee80211_data_header_t) +
	    ARRAY_SIZE(rfc1042_header) + head_space,
	    data + drop_bytes, size - drop_bytes);

	ieee80211_data_header_t *data_header =
	    (ieee80211_data_header_t *) complete_buffer;
	data_header->frame_ctrl =
	    uint16_t_le2host(IEEE80211_DATA_FRAME) |
	    uint16_t_le2host(IEEE80211_DATA_DATA_FRAME) |
	    uint16_t_le2host(IEEE80211_FRAME_CTRL_TODS) |
	    crypto;
	data_header->seq_ctrl = ieee80211_get_sequence_number(ieee80211_dev);

	/* BSSID, SA, DA. */
	memcpy(data_header->address1, auth_data->bssid.address, ETH_ADDR);
	memcpy(data_header->address2, data + ETH_ADDR, ETH_ADDR);
	memcpy(data_header->address3, data, ETH_ADDR);

	if (add_mic) {
		size_t size_wo_mic = complete_size - MIC_LENGTH;
		uint8_t *tx_mic = ieee80211_dev->bssid_info.ptk +
		    TK_OFFSET + IEEE80211_TKIP_TX_MIC_OFFSET;
		ieee80211_michael_mic(tx_mic, complete_buffer, size_wo_mic,
		    complete_buffer + size_wo_mic);
	}

	ieee80211_dev->ops->tx_handler(ieee80211_dev,
	    complete_buffer, complete_size);

	free(complete_buffer);
}

/** Fill out IEEE 802.11 device functions implementations.
 *
 * @param ieee80211_dev   IEEE 802.11 device.
 * @param ieee80211_ops   Callbacks implementation.
 * @param ieee80211_iface Interface functions implementation.
 * @param nic_iface       NIC interface functions implementation.
 * @param nic_dev_ops     NIC device functions implementation.
 *
 * @return EINVAL when missing pointer to ieee80211_ops
 *         or ieee80211_iface, otherwise EOK.
 *
 */
static errno_t ieee80211_implement(ieee80211_dev_t *ieee80211_dev,
    ieee80211_ops_t *ieee80211_ops, ieee80211_iface_t *ieee80211_iface,
    nic_iface_t *nic_iface, ddf_dev_ops_t *nic_dev_ops)
{
	if (ieee80211_ops) {
		if (!ieee80211_ops->start)
			ieee80211_ops->start = ieee80211_start_impl;

		if (!ieee80211_ops->tx_handler)
			ieee80211_ops->tx_handler = ieee80211_tx_handler_impl;

		if (!ieee80211_ops->set_freq)
			ieee80211_ops->set_freq = ieee80211_set_freq_impl;

		if (!ieee80211_ops->bssid_change)
			ieee80211_ops->bssid_change = ieee80211_bssid_change_impl;

		if (!ieee80211_ops->key_config)
			ieee80211_ops->key_config = ieee80211_key_config_impl;

		if (!ieee80211_ops->scan)
			ieee80211_ops->scan = ieee80211_scan_impl;
	} else
		return EINVAL;

	ieee80211_dev->ops = ieee80211_ops;

	if (ieee80211_iface) {
		if (nic_dev_ops)
			if (!nic_dev_ops->interfaces[IEEE80211_DEV_IFACE])
				nic_dev_ops->interfaces[IEEE80211_DEV_IFACE] =
				    ieee80211_iface;

		if (!ieee80211_iface->get_scan_results)
			ieee80211_iface->get_scan_results =
			    ieee80211_get_scan_results_impl;

		if (!ieee80211_iface->connect)
			ieee80211_iface->connect = ieee80211_connect_impl;

		if (!ieee80211_iface->disconnect)
			ieee80211_iface->disconnect = ieee80211_disconnect_impl;
	} else
		return EINVAL;

	if (nic_dev_ops) {
		if (!nic_dev_ops->open)
			nic_dev_ops->open = ieee80211_open;
	} else
		return EINVAL;

	ieee80211_dev->iface = ieee80211_iface;

	nic_driver_implement(NULL, nic_dev_ops, nic_iface);

	return EOK;
}

/** Allocate IEEE802.11 device structure.
 *
 * @return Pointer to allocated IEEE802.11 device structure.
 *
 */
ieee80211_dev_t *ieee80211_device_create(void)
{
	return calloc(1, sizeof(ieee80211_dev_t));
}

/** Initialize an IEEE802.11 framework structure.
 *
 * @param ieee80211_dev Device structure to initialize.
 * @param ddf_dev       Pointer to backing DDF device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, ddf_dev_t *ddf_dev)
{
	ieee80211_dev->ddf_dev = ddf_dev;
	ieee80211_dev->started = false;
	ieee80211_dev->ready = false;
	ieee80211_dev->using_hw_key = false;
	ieee80211_dev->pending_conn_req = false;
	ieee80211_dev->current_op_mode = IEEE80211_OPMODE_STATION;
	ieee80211_dev->current_auth_phase = IEEE80211_AUTH_DISCONNECTED;

	memcpy(ieee80211_dev->bssid_mask.address, ieee80211_broadcast_mac_addr,
	    ETH_ADDR);

	ieee80211_scan_result_list_init(&ieee80211_dev->ap_list);

	fibril_mutex_initialize(&ieee80211_dev->scan_mutex);
	fibril_mutex_initialize(&ieee80211_dev->gen_mutex);
	fibril_condvar_initialize(&ieee80211_dev->gen_cond);

	/* Bind NIC to device */
	nic_t *nic = nic_create_and_bind(ddf_dev);
	if (!nic)
		return ENOMEM;

	nic_set_specific(nic, ieee80211_dev);

	return EOK;
}

/** IEEE802.11 WiFi framework initialization.
 *
 * @param ieee80211_dev   Device structure to initialize.
 * @param ieee80211_ops   Structure with implemented IEEE802.11
 *                        device operations.
 * @param ieee80211_iface Structure with implemented IEEE802.11
 *                        interface operations.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_init(ieee80211_dev_t *ieee80211_dev,
    ieee80211_ops_t *ieee80211_ops, ieee80211_iface_t *ieee80211_iface,
    nic_iface_t *ieee80211_nic_iface, ddf_dev_ops_t *ieee80211_nic_dev_ops)
{
	errno_t rc = ieee80211_implement(ieee80211_dev,
	    ieee80211_ops, ieee80211_iface,
	    ieee80211_nic_iface, ieee80211_nic_dev_ops);
	if (rc != EOK)
		return rc;

	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);

	/* TODO: Set NIC handlers here. */
	nic_set_send_frame_handler(nic, ieee80211_send_frame);

	ddf_fun_t *fun = ddf_fun_create(ieee80211_dev->ddf_dev, fun_exposed,
	    "port0");
	if (fun == NULL)
		return EINVAL;

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

/** Convert frequency value to channel number.
 *
 * @param freq IEEE 802.11 operating frequency.
 *
 * @return Operating channel number.
 *
 */
static uint8_t ieee80211_freq_to_channel(uint16_t freq)
{
	return (freq - IEEE80211_FIRST_FREQ) / IEEE80211_CHANNEL_GAP + 1;
}

static void ieee80211_prepare_ie_header(void **ie_header,
    uint8_t id, uint8_t length, void *data)
{
	ieee80211_ie_header_t *header =
	    (ieee80211_ie_header_t *) *ie_header;

	header->element_id = id;
	header->length = length;

	memcpy(*ie_header + sizeof(ieee80211_ie_header_t), data, length);

	*ie_header = (void *) ((void *) header +
	    sizeof(ieee80211_ie_header_t) + length);
}

/** Probe request implementation.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param ssid          Probing SSID or NULL if broadcast.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_probe_request(ieee80211_dev_t *ieee80211_dev, char *ssid)
{
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);

	size_t ssid_data_size = (ssid != NULL) ? str_size(ssid) : 0;
	size_t channel_data_size = 1;

	uint8_t channel =
	    ieee80211_freq_to_channel(ieee80211_dev->current_freq);

	/*
	 * 4 headers - (ssid, rates, ext rates, current channel)
	 * and their data lengths.
	 */
	size_t payload_size =
	    sizeof(ieee80211_ie_header_t) * 4 +
	    ssid_data_size +
	    IEEE80211_DATA_RATES_SIZE + IEEE80211_EXT_DATA_RATES_SIZE +
	    channel_data_size;

	size_t buffer_size = sizeof(ieee80211_mgmt_header_t) + payload_size;
	void *buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 0, buffer_size);

	ieee80211_mgmt_header_t *mgmt_header =
	    (ieee80211_mgmt_header_t *) buffer;

	mgmt_header->frame_ctrl =
	    host2uint16_t_le(IEEE80211_MGMT_FRAME |
	    IEEE80211_MGMT_PROBE_REQ_FRAME);
	memcpy(mgmt_header->dest_addr, ieee80211_broadcast_mac_addr, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, ieee80211_broadcast_mac_addr, ETH_ADDR);
	mgmt_header->seq_ctrl =
	    host2uint16_t_le(ieee80211_get_sequence_number(ieee80211_dev));

	/* Jump to payload. */
	void *it = (void *) buffer + sizeof(ieee80211_mgmt_header_t);
	ieee80211_prepare_ie_header(&it, IEEE80211_SSID_IE, ssid_data_size,
	    (void *) ssid);
	ieee80211_prepare_ie_header(&it, IEEE80211_RATES_IE,
	    IEEE80211_DATA_RATES_SIZE, (void *) &ieee80211bg_data_rates);
	ieee80211_prepare_ie_header(&it, IEEE80211_EXT_RATES_IE,
	    IEEE80211_EXT_DATA_RATES_SIZE,
	    (void *) &ieee80211bg_data_rates[IEEE80211_DATA_RATES_SIZE]);
	ieee80211_prepare_ie_header(&it, IEEE80211_CHANNEL_IE,
	    channel_data_size, (void *) &channel);

	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);

	free(buffer);

	return EOK;
}

/** IEEE 802.11 authentication implementation.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_authenticate(ieee80211_dev_t *ieee80211_dev)
{
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);

	ieee80211_scan_result_t *auth_data =
	    &ieee80211_dev->bssid_info.res_link->scan_result;

	size_t buffer_size = sizeof(ieee80211_mgmt_header_t) +
	    sizeof(ieee80211_auth_body_t);

	void *buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 0, buffer_size);

	ieee80211_mgmt_header_t *mgmt_header =
	    (ieee80211_mgmt_header_t *) buffer;

	mgmt_header->frame_ctrl =
	    host2uint16_t_le(IEEE80211_MGMT_FRAME |
	    IEEE80211_MGMT_AUTH_FRAME);
	memcpy(mgmt_header->dest_addr, auth_data->bssid.address, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, auth_data->bssid.address, ETH_ADDR);

	ieee80211_auth_body_t *auth_body =
	    (ieee80211_auth_body_t *)
	    (buffer + sizeof(ieee80211_mgmt_header_t));
	auth_body->auth_alg = host2uint16_t_le(0);
	auth_body->auth_trans_no = host2uint16_t_le(1);

	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);

	free(buffer);

	return EOK;
}

/** IEEE 802.11 association implementation.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param password      Passphrase to be used in encrypted communication
 *                      or NULL for open networks.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_associate(ieee80211_dev_t *ieee80211_dev, char *password)
{
	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);

	ieee80211_scan_result_link_t *auth_link =
	    ieee80211_dev->bssid_info.res_link;

	ieee80211_scan_result_t *auth_data = &auth_link->scan_result;

	size_t ssid_data_size = str_size(auth_data->ssid);

	size_t payload_size =
	    sizeof(ieee80211_ie_header_t) * 3 +
	    ssid_data_size +
	    IEEE80211_DATA_RATES_SIZE +
	    IEEE80211_EXT_DATA_RATES_SIZE;

	size_t buffer_size =
	    sizeof(ieee80211_mgmt_header_t) +
	    sizeof(ieee80211_assoc_req_body_t) +
	    payload_size;

	if ((auth_data->security.type == IEEE80211_SECURITY_WPA) ||
	    (auth_data->security.type == IEEE80211_SECURITY_WPA2))
		buffer_size += auth_link->auth_ie_len;

	void *buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 0, buffer_size);

	ieee80211_mgmt_header_t *mgmt_header =
	    (ieee80211_mgmt_header_t *) buffer;

	mgmt_header->frame_ctrl =
	    host2uint16_t_le(IEEE80211_MGMT_FRAME |
	    IEEE80211_MGMT_ASSOC_REQ_FRAME);
	memcpy(mgmt_header->dest_addr, auth_data->bssid.address, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, auth_data->bssid.address, ETH_ADDR);

	ieee80211_assoc_req_body_t *assoc_body =
	    (ieee80211_assoc_req_body_t *)
	    (buffer + sizeof(ieee80211_mgmt_header_t));
	assoc_body->listen_interval = host2uint16_t_le(1);

	/* Jump to payload. */
	void *it = buffer + sizeof(ieee80211_mgmt_header_t) +
	    sizeof(ieee80211_assoc_req_body_t);
	ieee80211_prepare_ie_header(&it, IEEE80211_SSID_IE,
	    ssid_data_size, (void *) auth_data->ssid);
	ieee80211_prepare_ie_header(&it, IEEE80211_RATES_IE,
	    IEEE80211_DATA_RATES_SIZE, (void *) &ieee80211bg_data_rates);
	ieee80211_prepare_ie_header(&it, IEEE80211_EXT_RATES_IE,
	    IEEE80211_EXT_DATA_RATES_SIZE,
	    (void *) &ieee80211bg_data_rates[IEEE80211_DATA_RATES_SIZE]);

	if (auth_data->security.type != IEEE80211_SECURITY_OPEN)
		assoc_body->capability |= host2uint16_t_le(CAP_SECURITY);

	if ((auth_data->security.type == IEEE80211_SECURITY_WPA) ||
	    (auth_data->security.type == IEEE80211_SECURITY_WPA2))
		memcpy(it, auth_link->auth_ie, auth_link->auth_ie_len);

	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);

	/*
	 * Save password to be used in eventual authentication handshake.
	 */
	memset(ieee80211_dev->bssid_info.password, 0, IEEE80211_MAX_PASSW_LEN);
	memcpy(ieee80211_dev->bssid_info.password, password,
	    str_size(password));

	free(buffer);

	return EOK;
}

/** IEEE 802.11 deauthentication implementation.
 *
 * Note: Expecting locked results_mutex or scan_mutex.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_deauthenticate(ieee80211_dev_t *ieee80211_dev)
{
	ieee80211_scan_result_t *auth_data =
	    &ieee80211_dev->bssid_info.res_link->scan_result;

	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);

	size_t buffer_size = sizeof(ieee80211_mgmt_header_t) +
	    sizeof(ieee80211_deauth_body_t);

	void *buffer = malloc(buffer_size);
	if (!buffer)
		return ENOMEM;

	memset(buffer, 0, buffer_size);

	ieee80211_mgmt_header_t *mgmt_header =
	    (ieee80211_mgmt_header_t *) buffer;

	mgmt_header->frame_ctrl =
	    host2uint16_t_le(IEEE80211_MGMT_FRAME |
	    IEEE80211_MGMT_DEAUTH_FRAME);
	memcpy(mgmt_header->dest_addr, auth_data->bssid.address, ETH_ADDR);
	memcpy(mgmt_header->src_addr, nic_address.address, ETH_ADDR);
	memcpy(mgmt_header->bssid, auth_data->bssid.address, ETH_ADDR);

	ieee80211_dev->ops->tx_handler(ieee80211_dev, buffer, buffer_size);

	free(buffer);

	ieee80211_dev->bssid_info.res_link = NULL;
	ieee80211_dev->ops->bssid_change(ieee80211_dev, false);

	if (ieee80211_query_using_key(ieee80211_dev))
		ieee80211_dev->ops->key_config(ieee80211_dev, NULL, false);

	ieee80211_set_auth_phase(ieee80211_dev, IEEE80211_AUTH_DISCONNECTED);

	return EOK;
}

static void ieee80211_process_auth_info(ieee80211_scan_result_link_t *ap_data,
    void *buffer)
{
	uint8_t *it = (uint8_t *) buffer;

	uint16_t *version = (uint16_t *) it;
	if (uint16_t_le2host(*version) != 0x1) {
		ap_data->scan_result.security.type = -1;
		return;
	}

	it += sizeof(uint16_t);

	uint32_t group_cipher = *(it + 3);
	switch (group_cipher) {
	case IEEE80211_AUTH_CIPHER_TKIP:
		ap_data->scan_result.security.group_alg =
		    IEEE80211_SECURITY_SUITE_TKIP;
		break;
	case IEEE80211_AUTH_CIPHER_CCMP:
		ap_data->scan_result.security.group_alg =
		    IEEE80211_SECURITY_SUITE_CCMP;
		break;
	default:
		ap_data->scan_result.security.group_alg = -1;
	}

	it += 4 * sizeof(uint8_t);

	uint16_t *pairwise_count = (uint16_t *) it;
	uint32_t pairwise_cipher = *(it + sizeof(uint16_t) + 3);
	switch (pairwise_cipher) {
	case IEEE80211_AUTH_CIPHER_TKIP:
		ap_data->scan_result.security.pair_alg =
		    IEEE80211_SECURITY_SUITE_TKIP;
		break;
	case IEEE80211_AUTH_CIPHER_CCMP:
		ap_data->scan_result.security.pair_alg =
		    IEEE80211_SECURITY_SUITE_CCMP;
		break;
	default:
		ap_data->scan_result.security.pair_alg = -1;
	}

	it += 2 * sizeof(uint16_t) +
	    uint16_t_le2host(*pairwise_count) * sizeof(uint32_t);

	uint32_t auth_suite = *(it + 3);
	switch (auth_suite) {
	case IEEE80211_AUTH_AKM_PSK:
		ap_data->scan_result.security.auth =
		    IEEE80211_SECURITY_AUTH_PSK;
		break;
	case IEEE80211_AUTH_AKM_8021X:
		ap_data->scan_result.security.auth =
		    IEEE80211_SECURITY_AUTH_8021X;
		break;
	default:
		ap_data->scan_result.security.auth = -1;
	}
}

static void copy_auth_ie(ieee80211_ie_header_t *ie_header,
    ieee80211_scan_result_link_t *ap_data, void *it)
{
	ap_data->auth_ie_len = ie_header->length +
	    sizeof(ieee80211_ie_header_t);

	memcpy(ap_data->auth_ie, it, ap_data->auth_ie_len);
}

static uint8_t *ieee80211_process_ies(ieee80211_dev_t *ieee80211_dev,
    ieee80211_scan_result_link_t *ap_data, void *buffer, size_t buffer_size)
{
	void *it = buffer;
	while ((it + sizeof(ieee80211_ie_header_t)) < buffer + buffer_size) {
		ieee80211_ie_header_t *ie_header =
		    (ieee80211_ie_header_t *) it;
		uint8_t *channel;
		uint32_t oui;

		switch (ie_header->element_id) {
		case IEEE80211_CHANNEL_IE:
			if (!ap_data)
				break;

			channel = (uint8_t *)
			    (it + sizeof(ieee80211_ie_header_t));
			ap_data->scan_result.channel = *channel;
			break;
		case IEEE80211_RSN_IE:
			if (!ap_data)
				break;

			ap_data->scan_result.security.type =
			    IEEE80211_SECURITY_WPA2;
			ieee80211_process_auth_info(ap_data,
			    it + sizeof(ieee80211_ie_header_t));
			copy_auth_ie(ie_header, ap_data, it);
			break;
		case IEEE80211_VENDOR_IE:
			oui = uint32be_from_seq(it +
			    sizeof(ieee80211_ie_header_t));

			if (oui == WPA_OUI) {
				if (!ap_data)
					break;

				/* Prefering WPA2. */
				if (ap_data->scan_result.security.type ==
				    IEEE80211_SECURITY_WPA2)
					break;

				ap_data->scan_result.security.type =
				    IEEE80211_SECURITY_WPA;

				ieee80211_process_auth_info(ap_data,
				    it + sizeof(ieee80211_ie_header_t) +
				    sizeof(uint32_t));
				copy_auth_ie(ie_header, ap_data, it);
			} else if (oui == GTK_OUI) {
				return it +
				    sizeof(ieee80211_ie_header_t) +
				    sizeof(uint32_t);
			}
		}

		it += sizeof(ieee80211_ie_header_t) + ie_header->length;
	}

	return NULL;
}

/** Process probe response and store results.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param mgmt_header   Pointer to start of management frame header.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ieee80211_process_probe_response(ieee80211_dev_t *ieee80211_dev,
    ieee80211_mgmt_header_t *mgmt_header, size_t buffer_size)
{
	ieee80211_beacon_start_t *beacon_body = (ieee80211_beacon_start_t *)
	    ((void *) mgmt_header + sizeof(ieee80211_mgmt_header_t));

	ieee80211_ie_header_t *ssid_ie_header = (ieee80211_ie_header_t *)
	    ((void *) beacon_body + sizeof(ieee80211_beacon_start_t));

	/* Not empty SSID. */
	if (ssid_ie_header->length > 0) {
		ieee80211_scan_result_list_t *result_list =
		    &ieee80211_dev->ap_list;

		uint8_t *ssid_start = (uint8_t *) ((void *) ssid_ie_header +
		    sizeof(ieee80211_ie_header_t));
		char ssid[IEEE80211_MAX_SSID_LENGTH];

		memcpy(ssid, ssid_start, ssid_ie_header->length);
		ssid[ssid_ie_header->length] = '\0';

		/* Check whether SSID is already in results. */
		ieee80211_scan_result_list_foreach(*result_list, result) {
			if (!str_cmp(ssid, result->scan_result.ssid)) {
				result->last_beacon = time(NULL);
				return EOK;
			}
		}

		/* Results are full. */
		if (result_list->size == IEEE80211_MAX_RESULTS_LENGTH - 1)
			return EOK;

		ieee80211_scan_result_link_t *ap_data =
		    malloc(sizeof(ieee80211_scan_result_link_t));
		if (!ap_data)
			return ENOMEM;

		memset(ap_data, 0, sizeof(ieee80211_scan_result_link_t));
		link_initialize(&ap_data->link);

		memcpy(ap_data->scan_result.bssid.address,
		    mgmt_header->bssid, ETH_ADDR);
		memcpy(ap_data->scan_result.ssid, ssid,
		    ssid_ie_header->length + 1);

		if (uint16_t_le2host(beacon_body->capability) & CAP_SECURITY) {
			ap_data->scan_result.security.type =
			    IEEE80211_SECURITY_WEP;
		} else {
			ap_data->scan_result.security.type =
			    IEEE80211_SECURITY_OPEN;
			ap_data->scan_result.security.auth = -1;
			ap_data->scan_result.security.pair_alg = -1;
			ap_data->scan_result.security.group_alg = -1;
		}

		void *rest_ies_start = ssid_start + ssid_ie_header->length;
		size_t rest_buffer_size =
		    buffer_size -
		    sizeof(ieee80211_mgmt_header_t) -
		    sizeof(ieee80211_beacon_start_t) -
		    sizeof(ieee80211_ie_header_t) -
		    ssid_ie_header->length;

		ieee80211_process_ies(ieee80211_dev, ap_data, rest_ies_start,
		    rest_buffer_size);

		ap_data->last_beacon = time(NULL);

		fibril_mutex_lock(&ieee80211_dev->ap_list.results_mutex);
		ieee80211_scan_result_list_append(result_list, ap_data);
		fibril_mutex_unlock(&ieee80211_dev->ap_list.results_mutex);
	}

	return EOK;
}

/** Process authentication response.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param mgmt_header   Pointer to start of management frame header.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ieee80211_process_auth_response(ieee80211_dev_t *ieee80211_dev,
    ieee80211_mgmt_header_t *mgmt_header)
{
	ieee80211_auth_body_t *auth_body =
	    (ieee80211_auth_body_t *)
	    ((void *) mgmt_header + sizeof(ieee80211_mgmt_header_t));

	if (auth_body->status != 0)
		ieee80211_set_auth_phase(ieee80211_dev,
		    IEEE80211_AUTH_DISCONNECTED);
	else
		ieee80211_set_auth_phase(ieee80211_dev,
		    IEEE80211_AUTH_AUTHENTICATED);

	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	fibril_condvar_signal(&ieee80211_dev->gen_cond);
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return EOK;
}

/** Process association response.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param mgmt_header   Pointer to start of management frame header.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ieee80211_process_assoc_response(ieee80211_dev_t *ieee80211_dev,
    ieee80211_mgmt_header_t *mgmt_header)
{
	ieee80211_assoc_resp_body_t *assoc_resp =
	    (ieee80211_assoc_resp_body_t *) ((void *) mgmt_header +
	    sizeof(ieee80211_mgmt_header_t));

	if (assoc_resp->status != 0)
		ieee80211_set_auth_phase(ieee80211_dev,
		    IEEE80211_AUTH_DISCONNECTED);
	else {
		ieee80211_dev->bssid_info.aid =
		    uint16_t_le2host(assoc_resp->aid);
		ieee80211_set_auth_phase(ieee80211_dev,
		    IEEE80211_AUTH_ASSOCIATED);
		ieee80211_dev->ops->bssid_change(ieee80211_dev, true);
	}

	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	fibril_condvar_signal(&ieee80211_dev->gen_cond);
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);

	return EOK;
}

static errno_t ieee80211_process_4way_handshake(ieee80211_dev_t *ieee80211_dev,
    void *buffer, size_t buffer_size)
{
	ieee80211_eapol_key_frame_t *key_frame =
	    (ieee80211_eapol_key_frame_t *) buffer;

	ieee80211_scan_result_link_t *auth_link =
	    ieee80211_dev->bssid_info.res_link;

	ieee80211_scan_result_t *auth_data = &auth_link->scan_result;

	/* We don't support 802.1X authentication yet. */
	if (auth_data->security.auth == IEEE80211_AUTH_AKM_8021X)
		return ENOTSUP;

	uint8_t *ptk = ieee80211_dev->bssid_info.ptk;
	uint8_t *gtk = ieee80211_dev->bssid_info.gtk;
	uint8_t gtk_id = 1;

	bool handshake_done = false;

	bool old_wpa =
	    auth_data->security.type == IEEE80211_SECURITY_WPA;

	bool key_phase =
	    uint16_t_be2host(key_frame->key_info) &
	    IEEE80211_EAPOL_KEY_KEYINFO_MIC;

	bool final_phase =
	    uint16_t_be2host(key_frame->key_info) &
	    IEEE80211_EAPOL_KEY_KEYINFO_SECURE;

	bool ccmp_used =
	    (auth_data->security.pair_alg == IEEE80211_SECURITY_SUITE_CCMP) ||
	    (auth_data->security.group_alg == IEEE80211_SECURITY_SUITE_CCMP);

	size_t ptk_key_length, gtk_key_length;
	hash_func_t mic_hash;
	if (ccmp_used)
		mic_hash = HASH_SHA1;
	else
		mic_hash = HASH_MD5;

	if (auth_data->security.pair_alg == IEEE80211_SECURITY_SUITE_CCMP)
		ptk_key_length = IEEE80211_PTK_CCMP_LENGTH;
	else
		ptk_key_length = IEEE80211_PTK_TKIP_LENGTH;

	if (auth_data->security.group_alg == IEEE80211_SECURITY_SUITE_CCMP)
		gtk_key_length = IEEE80211_GTK_CCMP_LENGTH;
	else
		gtk_key_length = IEEE80211_GTK_TKIP_LENGTH;

	size_t output_size =
	    sizeof(eth_header_t) +
	    sizeof(ieee80211_eapol_key_frame_t);

	if (!(uint16_t_be2host(key_frame->key_info) &
	    IEEE80211_EAPOL_KEY_KEYINFO_MIC))
		output_size += auth_link->auth_ie_len;

	nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
	nic_address_t nic_address;
	nic_query_address(nic, &nic_address);

	void *output_buffer = malloc(output_size);
	if (!output_buffer)
		return ENOMEM;

	memset(output_buffer, 0, output_size);

	/* Setup ethernet header. */
	eth_header_t *eth_header = (eth_header_t *) output_buffer;
	memcpy(eth_header->dest_addr, auth_data->bssid.address, ETH_ADDR);
	memcpy(eth_header->src_addr, nic_address.address, ETH_ADDR);
	eth_header->proto = host2uint16_t_be(ETH_TYPE_PAE);

	ieee80211_eapol_key_frame_t *output_key_frame =
	    (ieee80211_eapol_key_frame_t *)
	    (output_buffer + sizeof(eth_header_t));

	/* Copy content of incoming EAPOL-Key frame. */
	memcpy((void *) output_key_frame, buffer,
	    sizeof(ieee80211_eapol_key_frame_t));

	output_key_frame->proto_version = 0x1;
	output_key_frame->body_length =
	    host2uint16_t_be(output_size - sizeof(eth_header_t) - 4);
	output_key_frame->key_info &=
	    ~host2uint16_t_be(IEEE80211_EAPOL_KEY_KEYINFO_ACK);

	if (key_phase) {
		output_key_frame->key_info &=
		    ~host2uint16_t_be(IEEE80211_EAPOL_KEY_KEYINFO_ENCDATA);
		output_key_frame->key_info &=
		    ~host2uint16_t_be(IEEE80211_EAPOL_KEY_KEYINFO_INSTALL);
		output_key_frame->key_data_length = 0;
		memset(output_key_frame->key_nonce, 0, 32);
		memset(output_key_frame->key_mic, 0, 16);
		memset(output_key_frame->key_rsc, 0, 8);
		memset(output_key_frame->eapol_key_iv, 0, 16);

		/* Derive GTK and save it. */
		if (final_phase) {
			uint16_t key_data_length =
			    uint16_t_be2host(key_frame->key_data_length);
			uint8_t key_data[key_data_length];
			uint8_t *data_ptr = (uint8_t *)
			    (buffer + sizeof(ieee80211_eapol_key_frame_t));

			errno_t rc;
			uint8_t work_key[32];

			if (ccmp_used) {
				rc = ieee80211_aes_key_unwrap(ptk + KEK_OFFSET,
				    data_ptr, key_data_length, key_data);
			} else {
				memcpy(work_key, key_frame->eapol_key_iv, 16);
				memcpy(work_key + 16, ptk + KEK_OFFSET, 16);
				rc = ieee80211_rc4_key_unwrap(work_key,
				    data_ptr, key_data_length, key_data);
			}

			if (rc == EOK) {
				uint8_t *key_data_ptr = old_wpa ? key_data :
				    ieee80211_process_ies(ieee80211_dev,
				    NULL, key_data, key_data_length);

				if (key_data_ptr) {
					uint8_t *key_ptr;

					if (old_wpa)
						key_ptr = key_data_ptr;
					else {
						gtk_id = *key_data_ptr & 0x3;
						key_ptr = key_data_ptr + 2;
					}

					memcpy(gtk, key_ptr, gtk_key_length);
					handshake_done = true;
				}
			}
		}
	} else {
		output_key_frame->key_info |=
		    host2uint16_t_be(IEEE80211_EAPOL_KEY_KEYINFO_MIC);
		output_key_frame->key_data_length =
		    host2uint16_t_be(auth_link->auth_ie_len);
		memcpy((void *) output_key_frame +
		    sizeof(ieee80211_eapol_key_frame_t),
		    auth_link->auth_ie, auth_link->auth_ie_len);

		/* Compute PMK. */
		uint8_t pmk[PBKDF2_KEY_LENGTH];
		pbkdf2((uint8_t *) ieee80211_dev->bssid_info.password,
		    str_size(ieee80211_dev->bssid_info.password),
		    (uint8_t *) auth_data->ssid,
		    str_size(auth_data->ssid), pmk);

		uint8_t *anonce = key_frame->key_nonce;

		/* Generate SNONCE. */
		uint8_t snonce[32];
		rnd_sequence(snonce, 32);

		memcpy(output_key_frame->key_nonce, snonce, 32);

		uint8_t *dest_addr = eth_header->dest_addr;
		uint8_t *src_addr = eth_header->src_addr;

		/* Derive PTK and save it. */
		uint8_t crypt_data[PRF_CRYPT_DATA_LENGTH];
		memcpy(crypt_data,
		    min_sequence(dest_addr, src_addr, ETH_ADDR), ETH_ADDR);
		memcpy(crypt_data + ETH_ADDR,
		    max_sequence(dest_addr, src_addr, ETH_ADDR), ETH_ADDR);
		memcpy(crypt_data + 2*ETH_ADDR,
		    min_sequence(anonce, snonce, 32), 32);
		memcpy(crypt_data + 2*ETH_ADDR + 32,
		    max_sequence(anonce, snonce, 32), 32);
		ieee80211_prf(pmk, crypt_data, ptk, ptk_key_length);
	}

	/* Compute MIC of key frame data from KCK part of PTK. */
	uint8_t mic[mic_hash];
	hmac(ptk, 16, (uint8_t *) output_key_frame,
	    output_size - sizeof(eth_header_t), mic, mic_hash);

	memcpy(output_key_frame->key_mic, mic, 16);

	ieee80211_send_frame(nic, output_buffer, output_size);

	free(output_buffer);

	ieee80211_key_config_t key_config;

	/* Insert Pairwise key. */
	if ((key_phase && old_wpa) || (final_phase && !old_wpa)) {
		key_config.suite = auth_data->security.pair_alg;
		key_config.flags =
		    IEEE80211_KEY_FLAG_TYPE_PAIRWISE;
		memcpy(key_config.data,
		    ptk + TK_OFFSET, ptk_key_length - TK_OFFSET);

		ieee80211_dev->ops->key_config(ieee80211_dev,
		    &key_config, true);
	}

	/* Insert Group key. */
	if (final_phase) {
		key_config.id = gtk_id;
		key_config.suite = auth_data->security.group_alg;
		key_config.flags = IEEE80211_KEY_FLAG_TYPE_GROUP;
		memcpy(key_config.data, gtk, gtk_key_length);

		ieee80211_dev->ops->key_config(ieee80211_dev,
		    &key_config, true);
	}

	/* Signal successful handshake completion. */
	if (handshake_done) {
		fibril_mutex_lock(&ieee80211_dev->gen_mutex);
		fibril_condvar_signal(&ieee80211_dev->gen_cond);
		fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
	}

	return EOK;
}

static errno_t ieee80211_process_eapol_frame(ieee80211_dev_t *ieee80211_dev,
    void *buffer, size_t buffer_size)
{
	ieee80211_eapol_key_frame_t *key_frame =
	    (ieee80211_eapol_key_frame_t *) buffer;

	if (ieee80211_is_eapol_key_frame(key_frame))
		return ieee80211_process_4way_handshake(ieee80211_dev, buffer,
		    buffer_size);

	return EOK;
}

/** Process data frame.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param buffer        Data buffer starting with IEEE 802.11 data header.
 * @param buffer_size   Size of buffer.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t ieee80211_process_data(ieee80211_dev_t *ieee80211_dev,
    void *buffer, size_t buffer_size)
{
	ieee80211_data_header_t *data_header =
	    (ieee80211_data_header_t *) buffer;

	if (ieee80211_has_data_frame(data_header->frame_ctrl)) {
		nic_t *nic = nic_get_from_ddf_dev(ieee80211_dev->ddf_dev);
		size_t strip_length = sizeof(ieee80211_data_header_t) +
		    ARRAY_SIZE(rfc1042_header);

		/* TODO: Different by used security alg. */
		/* TODO: Trim frame by used security alg. */
		// TODO: Distinguish used key (pair/group) by dest address ?
		if (ieee80211_is_encrypted_frame(data_header->frame_ctrl))
			strip_length += 8;

		/* Process 4-way authentication handshake. */
		uint16_t *proto = (uint16_t *) (buffer + strip_length);
		if (uint16_t_be2host(*proto) == ETH_TYPE_PAE)
			return ieee80211_process_eapol_frame(ieee80211_dev,
			    buffer + strip_length + sizeof(uint16_t),
			    buffer_size - strip_length - sizeof(uint16_t));

		/*
		 * Note: ETH protocol ID is already there, so we don't create
		 * whole ETH header.
		 */
		size_t frame_size =
		    buffer_size - strip_length + sizeof(eth_header_t) - 2;
		nic_frame_t *frame = nic_alloc_frame(nic, frame_size);

		if(frame == NULL)
			return ENOMEM;

		uint8_t *src_addr =
		    ieee80211_is_fromds_frame(data_header->frame_ctrl) ?
		    data_header->address3 : data_header->address2;
		uint8_t *dest_addr =
		    ieee80211_is_tods_frame(data_header->frame_ctrl) ?
		    data_header->address3 : data_header->address1;

		eth_header_t *eth_header = (eth_header_t *) frame->data;
		memcpy(eth_header->src_addr, src_addr, ETH_ADDR);
		memcpy(eth_header->dest_addr, dest_addr, ETH_ADDR);

		memcpy(frame->data + sizeof(eth_header_t) - 2,
		    buffer + strip_length, buffer_size - strip_length);

		nic_received_frame(nic, frame);
	}

	return EOK;
}

/** IEEE 802.11 RX frames handler.
 *
 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
 * @param buffer        Buffer with data.
 * @param buffer_size   Size of buffer.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
errno_t ieee80211_rx_handler(ieee80211_dev_t *ieee80211_dev, void *buffer,
    size_t buffer_size)
{
	uint16_t frame_ctrl = *((uint16_t *) buffer);

	if (ieee80211_is_mgmt_frame(frame_ctrl)) {
		ieee80211_mgmt_header_t *mgmt_header =
		    (ieee80211_mgmt_header_t *) buffer;

		if ((ieee80211_is_probe_response_frame(mgmt_header->frame_ctrl)) ||
		    (ieee80211_is_beacon_frame(mgmt_header->frame_ctrl)))
			return ieee80211_process_probe_response(ieee80211_dev,
			    mgmt_header, buffer_size);

		if (ieee80211_is_auth_frame(mgmt_header->frame_ctrl))
			return ieee80211_process_auth_response(ieee80211_dev,
			    mgmt_header);

		if (ieee80211_is_assoc_response_frame(mgmt_header->frame_ctrl))
			return ieee80211_process_assoc_response(ieee80211_dev,
			    mgmt_header);
	} else if (ieee80211_is_data_frame(frame_ctrl))
		return ieee80211_process_data(ieee80211_dev, buffer,
		    buffer_size);

	return EOK;
}

/** @}
 */
