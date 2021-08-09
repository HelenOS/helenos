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

/** @addtogroup libdevice
 *  @{
 */

/** @file ieee80211.h
 *  IEEE 802.11 common definitions.
 */

#ifndef LIBDEVICE_IEEE80211_H
#define LIBDEVICE_IEEE80211_H

#include <adt/list.h>
#include <nic/nic.h>
#include <time.h>

/** Max length of scan results array. */
#define IEEE80211_MAX_RESULTS_LENGTH  32

/** Max SSID length including null character. */
#define IEEE80211_MAX_SSID_LENGTH  35

/** WiFi security authentication method indicator. */
typedef enum {
	IEEE80211_SECURITY_OPEN,
	IEEE80211_SECURITY_WEP,
	IEEE80211_SECURITY_WPA,
	IEEE80211_SECURITY_WPA2
} ieee80211_security_type_t;

/** WiFi security suite indicator. */
typedef enum {
	IEEE80211_SECURITY_SUITE_WEP40,
	IEEE80211_SECURITY_SUITE_WEP104,
	IEEE80211_SECURITY_SUITE_CCMP,
	IEEE80211_SECURITY_SUITE_TKIP
} ieee80211_security_suite_t;

/** WiFi security authentication method indicator. */
typedef enum {
	IEEE80211_SECURITY_AUTH_PSK,
	IEEE80211_SECURITY_AUTH_8021X
} ieee80211_security_auth_t;

/** Structure for indicating security network security settings. */
typedef struct {
	int type;
	int group_alg;
	int pair_alg;
	int auth;
} ieee80211_security_t;

/** Structure with scan result info. */
typedef struct {
	nic_address_t bssid;
	char ssid[IEEE80211_MAX_SSID_LENGTH];
	uint8_t channel;
	ieee80211_security_t security;
} ieee80211_scan_result_t;

/** Array of scan results info. */
typedef struct {
	uint8_t length;
	ieee80211_scan_result_t results[IEEE80211_MAX_RESULTS_LENGTH];
} ieee80211_scan_results_t;

#endif

/** @}
 */
