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

#include <str.h>
#include <errno.h>

#include <ieee80211_private.h>
#include <ieee80211_iface_impl.h>

/** @addtogroup libieee80211
 * @{
 */

/** @file ieee80211_iface_impl.c
 * 
 * IEEE 802.11 default interface functions implementation.
 */

/**
 * Implementation of fetching scan results.
 * 
 * @param fun Device function.
 * @param results Structure where should be stored scan results.
 * 
 * @return EOK. 
 */
int ieee80211_get_scan_results_impl(ddf_fun_t *fun, 
	ieee80211_scan_results_t *results, bool now)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	ieee80211_dev_t *ieee80211_dev = nic_get_specific(nic_data);
	
	if(!ieee80211_is_ready(ieee80211_dev))
		return EREFUSED;
	
	fibril_mutex_lock(&ieee80211_dev->ap_list.scan_mutex);
	time_t scan_span = time(NULL) - ieee80211_dev->ap_list.last_scan;
	fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
	
	if(now || scan_span > MAX_SCAN_SPAN_SEC) {
		ieee80211_dev->ops->scan(ieee80211_dev);
	}
	
	fibril_mutex_lock(&ieee80211_dev->ap_list.scan_mutex);
	if(results) {
		ieee80211_scan_result_list_t *result_list =
			&ieee80211_dev->ap_list;
		
		int i = 0;
		ieee80211_scan_result_list_foreach(*result_list, result) {
			memcpy(&results->results[i], 
				&result->scan_result, 
				sizeof(ieee80211_scan_result_t));
			i++;
		}
		
		results->length = i;
	}
	fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
	
	return EOK;
}

static uint16_t ieee80211_channel_to_freq(uint8_t channel)
{
	return IEEE80211_CHANNEL_GAP * (channel - 1) + IEEE80211_FIRST_FREQ;
}

/**
 * Working procedure of connect function.
 * 
 * @param ieee80211_dev Pointer to IEEE 802.11 device.
 * @param auth_data Selected AP data we want to connect to.
 * 
 * @return EOK if everything OK, ETIMEOUT when timeout during authenticating.
 */
static int ieee80211_connect_proc(ieee80211_dev_t *ieee80211_dev,
	ieee80211_scan_result_link_t *auth_data, char *password)
{
	int rc;
	
	ieee80211_dev->bssid_info.res_link = auth_data;
	
	/* Set channel. */
	rc = ieee80211_dev->ops->set_freq(ieee80211_dev, 
		ieee80211_channel_to_freq(auth_data->scan_result.channel));
	if(rc != EOK)
		return rc;
	
	/* Try to authenticate. */
	ieee80211_authenticate(ieee80211_dev);
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	rc = fibril_condvar_wait_timeout(&ieee80211_dev->gen_cond,
			&ieee80211_dev->gen_mutex,
			AUTH_TIMEOUT);
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
	if(rc != EOK)
		return rc;
	if(ieee80211_dev->current_auth_phase != IEEE80211_AUTH_AUTHENTICATED)
		return EINVAL;
	
	/* Try to associate. */
	ieee80211_associate(ieee80211_dev, password);
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	rc = fibril_condvar_wait_timeout(&ieee80211_dev->gen_cond,
			&ieee80211_dev->gen_mutex,
			AUTH_TIMEOUT);
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
	if(rc != EOK)
		return rc;
	if(ieee80211_dev->current_auth_phase != IEEE80211_AUTH_ASSOCIATED)
		return EINVAL;
	
	/* On open network, we are finished. */
	if(auth_data->scan_result.security.type == IEEE80211_SECURITY_OPEN)
		return EOK;
	
	/* Otherwise wait for 4-way handshake to complete. */
	fibril_mutex_lock(&ieee80211_dev->gen_mutex);
	rc = fibril_condvar_wait_timeout(&ieee80211_dev->gen_cond,
			&ieee80211_dev->gen_mutex,
			HANDSHAKE_TIMEOUT);
	fibril_mutex_unlock(&ieee80211_dev->gen_mutex);
	if(rc != EOK)
		return rc;
	if(ieee80211_dev->current_auth_phase != IEEE80211_AUTH_ASSOCIATED)
		return EINVAL;
	
	return EOK;
}

/**
 * Implementation of connecting to specified SSID.
 * 
 * @param fun Device function.
 * @param ssid_start SSID prefix of access point we want to connect to.
 * 
 * @return EOK if everything OK, ETIMEOUT when timeout during authenticating,
 * EINVAL when SSID not in scan results list, EPERM when incorrect password
 * passed.
 */
int ieee80211_connect_impl(ddf_fun_t *fun, char *ssid_start, char *password)
{
	assert(ssid_start);
	assert(password);
	
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	ieee80211_dev_t *ieee80211_dev = nic_get_specific(nic_data);
	
	if(!ieee80211_is_ready(ieee80211_dev))
		return EREFUSED;
	
	if(ieee80211_is_connected(ieee80211_dev)) {
		int rc = ieee80211_dev->iface->disconnect(fun);
		if(rc != EOK)
			return rc;
	}
	
	fibril_mutex_lock(&ieee80211_dev->ap_list.scan_mutex);

	ieee80211_scan_result_list_foreach(ieee80211_dev->ap_list, result) {
		if(!str_lcmp(ssid_start, 
			result->scan_result.ssid, 
			str_size(ssid_start))) {
			fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
			return ieee80211_connect_proc(ieee80211_dev, result,
				password);
		}
	}
	
	fibril_mutex_unlock(&ieee80211_dev->ap_list.scan_mutex);
	
	return EINVAL;
}

/**
 * Implementation of disconnecting device from network.
 * 
 * @param fun Device function.
 * 
 * @return EOK if everything OK, EINVAL when not connected to any network.
 */
int ieee80211_disconnect_impl(ddf_fun_t *fun)
{
	nic_t *nic_data = nic_get_from_ddf_fun(fun);
	ieee80211_dev_t *ieee80211_dev = nic_get_specific(nic_data);
	
	if(!ieee80211_is_ready(ieee80211_dev))
		return EREFUSED;
	
	if(!ieee80211_is_connected(ieee80211_dev)) {
		return EINVAL;
	} else {
		return ieee80211_deauthenticate(ieee80211_dev);
	}
}

/** @}
 */