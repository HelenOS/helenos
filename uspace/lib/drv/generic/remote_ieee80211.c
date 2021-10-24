/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libdrv
 * @{
 */
/**
 * @file
 * @brief Driver-side RPC skeletons for IEEE 802.11 interface
 */

#include <errno.h>
#include <macros.h>
#include <str.h>
#include <inet/dhcp.h>
#include <inet/inetcfg.h>
#include "ops/ieee80211.h"
#include "ieee80211_iface.h"
#include "nic_iface.h"

#define MAX_STRING_SIZE  32

/** IEEE 802.11 RPC functions IDs. */
typedef enum {
	IEEE80211_GET_SCAN_RESULTS,
	IEEE80211_CONNECT,
	IEEE80211_DISCONNECT
} ieee80211_funcs_t;

/** Get scan results from IEEE 802.11 device
 *
 * @param[in]  dev_sess Device session.
 * @param[out] results  Structure where to put scan results.
 *
 * @return EOK If the operation was successfully completed,
 *         error code otherwise.
 *
 */
errno_t ieee80211_get_scan_results(async_sess_t *dev_sess,
    ieee80211_scan_results_t *results, bool now)
{
	assert(results);

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t aid = async_send_2(exch, DEV_IFACE_ID(IEEE80211_DEV_IFACE),
	    IEEE80211_GET_SCAN_RESULTS, now, NULL);
	errno_t rc = async_data_read_start(exch, results,
	    sizeof(ieee80211_scan_results_t));
	async_exchange_end(exch);

	errno_t res;
	async_wait_for(aid, &res);

	if (res != EOK)
		return (errno_t) res;

	return rc;
}

// XXX This is wrong. Wifi should not have anything to do with IP links
static sysarg_t get_link_id(uint8_t *mac)
{
	sysarg_t *link_list;
	inet_link_info_t link_info;
	eth_addr_t eth_addr;
	size_t count;

	eth_addr_decode(mac, &eth_addr);

	errno_t rc = inetcfg_get_link_list(&link_list, &count);
	if (rc != EOK)
		return -1;

	for (size_t i = 0; i < count; i++) {
		rc = inetcfg_link_get(link_list[i], &link_info);
		if (rc != EOK)
			return -1;

		if (eth_addr_compare(&eth_addr, &link_info.mac_addr) == 0)
			return link_list[i];
	}

	return -1;
}

/** Connect to specified network.
 *
 * @param[in] dev_sess   Device session.
 * @param[in] ssid_start Network SSID prefix.
 * @param[in] password   Network password (pass empty string if not needed).
 *
 * @return EOK If the operation was successfully completed,
 *         error code otherwise.
 *
 */
errno_t ieee80211_connect(async_sess_t *dev_sess, char *ssid_start, char *password)
{
	assert(ssid_start);

	errno_t rc_orig;

	async_exch_t *exch = async_exchange_begin(dev_sess);

	aid_t aid = async_send_1(exch, DEV_IFACE_ID(IEEE80211_DEV_IFACE),
	    IEEE80211_CONNECT, NULL);

	errno_t rc = async_data_write_start(exch, ssid_start,
	    str_size(ssid_start) + 1);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(aid, &rc_orig);

		if (rc_orig == EOK)
			return (errno_t) rc;

		return (errno_t) rc_orig;
	}

	// FIXME: Typecasting string literal
	if (password == NULL)
		password = (char *) "";

	rc = async_data_write_start(exch, password, str_size(password) + 1);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(aid, &rc_orig);

		if (rc_orig == EOK)
			return (errno_t) rc;

		return (errno_t) rc_orig;
	}

	async_exchange_end(exch);

	async_wait_for(aid, &rc);
	if (rc != EOK)
		return rc;

	// XXX This is wrong. Wifi should not initiate DHCP

	/* Send DHCP discover. */
	nic_address_t wifi_mac;
	rc = nic_get_address(dev_sess, &wifi_mac);
	if (rc != EOK)
		return rc;

	sysarg_t link_id = get_link_id(wifi_mac.address);
	if (link_id == ((sysarg_t) -1))
		return EINVAL;

	rc = dhcp_discover(link_id);

	return (errno_t) rc;
}

/** Disconnect device from network.
 *
 * @param[in] dev_sess Device session.
 *
 * @return EOK If the operation was successfully completed,
 *         error code otherwise.
 *
 */
errno_t ieee80211_disconnect(async_sess_t *dev_sess)
{
	async_exch_t *exch = async_exchange_begin(dev_sess);
	errno_t rc = async_req_1_0(exch, DEV_IFACE_ID(IEEE80211_DEV_IFACE),
	    IEEE80211_DISCONNECT);
	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	nic_address_t wifi_mac;
	rc = nic_get_address(dev_sess, &wifi_mac);
	if (rc != EOK)
		return rc;

	eth_addr_t eth_addr;
	eth_addr_decode(wifi_mac.address, &eth_addr);

	inet_link_info_t link_info;
	inet_addr_info_t addr_info;
	inet_sroute_info_t route_info;
	sysarg_t *addr_list;
	sysarg_t *route_list;
	size_t count;

	/// XXX This is wrong. Wifi should do nothing with DHCP

	/* Remove previous DHCP address. */
	rc = inetcfg_get_addr_list(&addr_list, &count);
	if (rc != EOK)
		return rc;

	for (size_t i = 0; i < count; i++) {
		rc = inetcfg_addr_get(addr_list[i], &addr_info);
		if (rc != EOK)
			return rc;

		rc = inetcfg_link_get(addr_info.ilink, &link_info);
		if (rc != EOK)
			return rc;

		if (eth_addr_compare(&eth_addr, &link_info.mac_addr) == 0) {
			if (str_test_prefix(addr_info.name, "dhcp")) {
				rc = inetcfg_addr_delete(addr_list[i]);
				if (rc != EOK)
					return rc;

				break;
			}
		}
	}

	/*
	 * TODO: At this moment there can be only one DHCP route,
	 * so it must be reimplemented after this limitation will be
	 * dropped.
	 */
	/* Remove previous DHCP static route. */
	rc = inetcfg_get_sroute_list(&route_list, &count);
	if (rc != EOK)
		return rc;

	for (size_t i = 0; i < count; i++) {
		rc = inetcfg_sroute_get(route_list[i], &route_info);
		if (rc != EOK)
			return rc;

		if (str_test_prefix(route_info.name, "dhcp")) {
			rc = inetcfg_sroute_delete(route_list[i]);
			if (rc != EOK)
				return rc;

			break;
		}
	}

	return rc;
}

static void remote_ieee80211_get_scan_results(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	ieee80211_iface_t *ieee80211_iface = (ieee80211_iface_t *) iface;
	assert(ieee80211_iface->get_scan_results);

	ieee80211_scan_results_t scan_results;
	memset(&scan_results, 0, sizeof(ieee80211_scan_results_t));

	bool now = ipc_get_arg2(call);

	errno_t rc = ieee80211_iface->get_scan_results(fun, &scan_results, now);
	if (rc == EOK) {
		ipc_call_t data;
		size_t max_len;
		if (!async_data_read_receive(&data, &max_len)) {
			async_answer_0(&data, EINVAL);
			async_answer_0(call, EINVAL);
			return;
		}

		if (max_len < sizeof(ieee80211_scan_results_t)) {
			async_answer_0(&data, ELIMIT);
			async_answer_0(call, ELIMIT);
			return;
		}

		async_data_read_finalize(&data, &scan_results,
		    sizeof(ieee80211_scan_results_t));
	}

	async_answer_0(call, rc);
}

static void remote_ieee80211_connect(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	ieee80211_iface_t *ieee80211_iface = (ieee80211_iface_t *) iface;
	assert(ieee80211_iface->connect);

	char ssid_start[MAX_STRING_SIZE];
	char password[MAX_STRING_SIZE];

	ipc_call_t data;
	size_t len;
	if (!async_data_write_receive(&data, &len)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (len > MAX_STRING_SIZE) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	errno_t rc = async_data_write_finalize(&data, ssid_start, len);
	if (rc != EOK) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (!async_data_write_receive(&data, &len)) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	if (len > MAX_STRING_SIZE) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	rc = async_data_write_finalize(&data, password, len);
	if (rc != EOK) {
		async_answer_0(&data, EINVAL);
		async_answer_0(call, EINVAL);
		return;
	}

	rc = ieee80211_iface->connect(fun, ssid_start, password);

	async_answer_0(call, rc);
}

static void remote_ieee80211_disconnect(ddf_fun_t *fun, void *iface,
    ipc_call_t *call)
{
	ieee80211_iface_t *ieee80211_iface = (ieee80211_iface_t *) iface;
	assert(ieee80211_iface->disconnect);
	errno_t rc = ieee80211_iface->disconnect(fun);
	async_answer_0(call, rc);
}

/** Remote IEEE 802.11 interface operations.
 *
 */
static const remote_iface_func_ptr_t remote_ieee80211_iface_ops[] = {
	[IEEE80211_GET_SCAN_RESULTS] = remote_ieee80211_get_scan_results,
	[IEEE80211_CONNECT] = remote_ieee80211_connect,
	[IEEE80211_DISCONNECT] = remote_ieee80211_disconnect
};

/** Remote IEEE 802.11 interface structure.
 *
 * Interface for processing request from remote
 * clients addressed to the IEEE 802.11 interface.
 *
 */
const remote_iface_t remote_ieee80211_iface = {
	.method_count = ARRAY_SIZE(remote_ieee80211_iface_ops),
	.methods = remote_ieee80211_iface_ops
};

/**
 * @}
 */
