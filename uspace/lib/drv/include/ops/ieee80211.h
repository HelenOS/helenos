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
/** @file ieee80211.h
 * @brief IEEE 802.11 WiFi interface definition
 */

#ifndef LIBDRV_OPS_IEEE80211_H_
#define LIBDRV_OPS_IEEE80211_H_

#include <ddf/driver.h>
#include <ieee80211/ieee80211.h>

/** IEEE 802.11 interface functions definition. */
typedef struct ieee80211_iface {
	/** Fetch scan results from IEEE 802.11 device.
	 *
	 * @param fun     IEEE 802.11 function.
	 * @param results Structure where to put scan results.
	 * @param now     Whether to initiate scan immediately.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*get_scan_results)(ddf_fun_t *, ieee80211_scan_results_t *, bool);
	
	/** Connect IEEE 802.11 device to specified network.
	 *
	 * @param fun      IEEE 802.11 function.
	 * @param ssid     Network SSID.
	 * @param password Network password (empty string if not needed).
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*connect)(ddf_fun_t *, char *, char *);
	
	/** Disconnect IEEE 802.11 device from network.
	 *
	 * @param fun IEEE 802.11 function.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*disconnect)(ddf_fun_t *);
} ieee80211_iface_t;

#endif

/**
 * @}
 */
