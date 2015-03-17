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

#include "ops/ieee80211.h"
#include "ieee80211_iface.h"

#define MAX_STRING_SIZE 32

/** IEEE 802.11 RPC functions IDs. */
typedef enum {
	IEEE80211_GET_SCAN_RESULTS,
	IEEE80211_CONNECT
} ieee80211_funcs_t;

/** Get scan results from IEEE 802.11 device
 *
 * @param[in] dev_sess Device session.
 * @param[out] results Structure where to put scan results.
 *
 * @return EOK If the operation was successfully completed,
 * negative error code otherwise.
 */
int ieee80211_get_scan_results(async_sess_t *dev_sess, 
	ieee80211_scan_results_t *results)
{
	assert(results);
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(IEEE80211_DEV_IFACE),
	    IEEE80211_GET_SCAN_RESULTS, NULL);
	int rc = async_data_read_start(exch, results, sizeof(ieee80211_scan_results_t));
	async_exchange_end(exch);

	sysarg_t res;
	async_wait_for(aid, &res);
	
	if (rc != EOK)
		return rc;
	
	return (int) res;
}

/** Connect to specified network.
 *
 * @param[in] dev_sess Device session.
 * @param[in] ssid Network SSID.
 * @param[in] password Network password (pass empty string if not needed).
 *
 * @return EOK If the operation was successfully completed,
 * negative error code otherwise.
 */
int ieee80211_connect(async_sess_t *dev_sess, char *ssid, char *password)
{
	assert(ssid);
	
	sysarg_t rc_orig;
	
	async_exch_t *exch = async_exchange_begin(dev_sess);
	
	aid_t aid = async_send_1(exch, DEV_IFACE_ID(IEEE80211_DEV_IFACE),
	    IEEE80211_CONNECT, NULL);
	
	sysarg_t rc = async_data_write_start(exch, ssid, str_size(ssid) + 1);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(aid, &rc_orig);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	if(password == NULL) {
		password = (char *) "";
	}
	
	rc = async_data_write_start(exch, password, str_size(password) + 1);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_wait_for(aid, &rc_orig);
		
		if (rc_orig == EOK)
			return (int) rc;
		else
			return (int) rc_orig;
	}
	
	async_exchange_end(exch);

	async_wait_for(aid, &rc);
	
	return (int) rc;
}

static void remote_ieee80211_get_scan_results(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	ieee80211_iface_t *ieee80211_iface = (ieee80211_iface_t *) iface;
	assert(ieee80211_iface->get_scan_results);
	
	ieee80211_scan_results_t scan_results;
	memset(&scan_results, 0, sizeof(ieee80211_scan_results_t));
	
	int rc = ieee80211_iface->get_scan_results(fun, &scan_results);
	if (rc == EOK) {
		ipc_callid_t data_callid;
		size_t max_len;
		if (!async_data_read_receive(&data_callid, &max_len)) {
			async_answer_0(data_callid, EINVAL);
			async_answer_0(callid, EINVAL);
			return;
		}
		
		if (max_len < sizeof(ieee80211_scan_results_t)) {
			async_answer_0(data_callid, ELIMIT);
			async_answer_0(callid, ELIMIT);
			return;
		}
		
		async_data_read_finalize(data_callid, &scan_results,
		    sizeof(ieee80211_scan_results_t));
	}
	
	async_answer_0(callid, rc);
}

static void remote_ieee80211_connect(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	ieee80211_iface_t *ieee80211_iface = (ieee80211_iface_t *) iface;
	assert(ieee80211_iface->connect);
	
	char ssid[MAX_STRING_SIZE];
	char password[MAX_STRING_SIZE];
	
	ipc_callid_t data_callid;
	size_t len;
	if (!async_data_write_receive(&data_callid, &len)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (len > MAX_STRING_SIZE) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	int rc = async_data_write_finalize(data_callid, ssid, len);
	if (rc != EOK) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (!async_data_write_receive(&data_callid, &len)) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	if (len > MAX_STRING_SIZE) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	rc = async_data_write_finalize(data_callid, password, len);
	if (rc != EOK) {
		async_answer_0(data_callid, EINVAL);
		async_answer_0(callid, EINVAL);
		return;
	}
	
	rc = ieee80211_iface->connect(fun, ssid, password);
	
	async_answer_0(callid, rc);
}

/** Remote IEEE 802.11 interface operations.
 *
 */
static const remote_iface_func_ptr_t remote_ieee80211_iface_ops[] = {
	[IEEE80211_GET_SCAN_RESULTS] = remote_ieee80211_get_scan_results,
	[IEEE80211_CONNECT] = remote_ieee80211_connect
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
