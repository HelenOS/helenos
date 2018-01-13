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

/**
 * @addtogroup libieee80211
 * @{
 */
/**
 * @file ieee80211.h
 * @brief Public header exposing IEEE 802.11 to drivers.
 */

#ifndef LIB_IEEE80211_H
#define LIB_IEEE80211_H

#include <ddf/driver.h>
#include <nic.h>
#include <ops/ieee80211.h>

#define DEVICE_CATEGORY_IEEE80211  "ieee80211"

struct ieee80211_dev;
typedef struct ieee80211_dev ieee80211_dev_t;

/** Initial channel frequency. */
#define IEEE80211_FIRST_FREQ  2412

/** Max supported channel frequency. */
#define IEEE80211_MAX_FREQ  2472

/** Gap between IEEE80211 channels in MHz. */
#define IEEE80211_CHANNEL_GAP  5

/** Max AMPDU factor. */
#define IEEE80211_MAX_AMPDU_FACTOR  13

/** Max authentication password length. */
#define IEEE80211_MAX_PASSW_LEN  64

/** IEEE 802.11 b/g supported data rates in units of 500 kb/s. */
static const uint8_t ieee80211bg_data_rates[] = {
	2, 4, 11, 12, 18, 22, 24, 36, 48, 72, 96, 108
};

/** Device operating modes. */
typedef enum {
	IEEE80211_OPMODE_ADHOC,
	IEEE80211_OPMODE_MESH,
	IEEE80211_OPMODE_AP,
	IEEE80211_OPMODE_STATION
} ieee80211_operating_mode_t;

/** Key flags. */
typedef enum {
	IEEE80211_KEY_FLAG_TYPE_PAIRWISE = 0x01,
	IEEE80211_KEY_FLAG_TYPE_GROUP = 0x02
} ieee80211_key_flags_t;

typedef enum {
	IEEE80211_TKIP_TX_MIC_OFFSET = 16,
	IEEE80211_TKIP_RX_MIC_OFFSET = 24
} ieee80211_tkip_mic_offset_t;

/** Key config structure. */
typedef struct {
	uint8_t id;
	uint8_t flags;
	ieee80211_security_suite_t suite;
	uint8_t data[32];
} ieee80211_key_config_t;

/** IEEE 802.11 callback functions. */
typedef struct {
	/** unction that is called at device initalization.
	 *
	 * This should get device into running state.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*start)(struct ieee80211_dev *);
	
	/** Scan neighborhood for networks.
	 *
	 * There should be implemented scanning of whole bandwidth.
	 * Incoming results are processed by IEEE 802.11 framework itself.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*scan)(struct ieee80211_dev *);
	
	/** Handler for TX frames to be send from device.
	 *
	 * This should be called for every frame that has to be send
	 * from IEEE 802.11 device.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param buffer        Buffer with data to be send.
	 * @param buffer_size   Size of buffer.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*tx_handler)(struct ieee80211_dev *, void *, size_t);
	
	/** Set device operating frequency to given value.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param freq          New device operating frequency.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*set_freq)(struct ieee80211_dev *, uint16_t);
	
	/** Callback to inform device about BSSID change.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param connected     True if connected to new BSSID, otherwise false.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*bssid_change)(struct ieee80211_dev *, bool);
	
	/** Callback to setup encryption key in IEEE 802.11 device.
	 *
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param key_conf      Key config structure.
	 * @param insert        True to insert this key to device,
	 *                      false to remove it.
	 *
	 * @return EOK if succeed, error code otherwise.
	 *
	 */
	errno_t (*key_config)(struct ieee80211_dev *,
	    ieee80211_key_config_t *key_conf, bool);
} ieee80211_ops_t;

/* Initialization functions. */
extern ieee80211_dev_t *ieee80211_device_create(void);
extern errno_t ieee80211_device_init(ieee80211_dev_t *, ddf_dev_t *);
extern errno_t ieee80211_init(ieee80211_dev_t *, ieee80211_ops_t *,
    ieee80211_iface_t *, nic_iface_t *, ddf_dev_ops_t *);

/* Getters & setters, queries & reports. */
extern void *ieee80211_get_specific(ieee80211_dev_t *);
extern void ieee80211_set_specific(ieee80211_dev_t *, void *);
extern ddf_dev_t *ieee80211_get_ddf_dev(ieee80211_dev_t *);
extern ieee80211_operating_mode_t
    ieee80211_query_current_op_mode(ieee80211_dev_t *);
extern uint16_t ieee80211_query_current_freq(ieee80211_dev_t *);
extern void ieee80211_query_bssid(ieee80211_dev_t *, nic_address_t *);
extern bool ieee80211_is_connected(ieee80211_dev_t *);
extern void ieee80211_report_current_op_mode(ieee80211_dev_t *,
    ieee80211_operating_mode_t);
extern void ieee80211_report_current_freq(ieee80211_dev_t *, uint16_t);
extern uint16_t ieee80211_get_aid(ieee80211_dev_t *);
extern int ieee80211_get_pairwise_security(ieee80211_dev_t *);
extern bool ieee80211_is_ready(ieee80211_dev_t *);
extern void ieee80211_set_ready(ieee80211_dev_t *, bool);
extern bool ieee80211_query_using_key(ieee80211_dev_t *);
extern void ieee80211_setup_key_confirm(ieee80211_dev_t *, bool);

extern bool ieee80211_is_data_frame(uint16_t);
extern bool ieee80211_is_mgmt_frame(uint16_t);
extern bool ieee80211_is_beacon_frame(uint16_t);
extern bool ieee80211_is_probe_response_frame(uint16_t);
extern bool ieee80211_is_auth_frame(uint16_t);
extern bool ieee80211_is_assoc_response_frame(uint16_t);

/* Worker functions. */
extern errno_t ieee80211_rx_handler(ieee80211_dev_t *, void *, size_t);

#endif

/** @}
 */
