/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
