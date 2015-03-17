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

/** @addtogroup libc
 *  @{
 */

/** @file ieee80211.h
 *  IEEE 802.11 common definitions.
 */

#ifndef LIBC_IEEE80211_H_
#define LIBC_IEEE80211_H_

#include <malloc.h>
#include <adt/list.h>
#include <nic/nic.h>

/* Max length of scan results array. */
#define IEEE80211_MAX_RESULTS_LENGTH 32

/* Max SSID length including null character. */
#define IEEE80211_MAX_SSID_LENGTH 35

/** Structure with scan result info. */
typedef struct {
	nic_address_t bssid;
	char ssid[IEEE80211_MAX_SSID_LENGTH];
} ieee80211_scan_result_t;

/** List of scan results info. */
typedef struct {
	uint8_t length;
	ieee80211_scan_result_t results[IEEE80211_MAX_RESULTS_LENGTH];
} ieee80211_scan_results_t;

#endif

/** @}
 */

