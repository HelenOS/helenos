/*
 * SPDX-FileCopyrightText: 2015 Jan Kolarik
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
