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

/** @addtogroup libnet
 *  @{
 */

/** @file ieee80211.h
 * 
 * IEEE 802.11 interface definition.
 */

#ifndef LIBNET_IEEE80211_H
#define LIBNET_IEEE80211_H

#include <ddf/driver.h>
#include <sys/types.h>
#include <nic.h>

/** Initial channel frequency. */
#define IEEE80211_FIRST_FREQ 2412

/** Max supported channel frequency. */
#define IEEE80211_MAX_FREQ 2472

/* Gap between IEEE80211 channels in MHz. */
#define IEEE80211_CHANNEL_GAP 5

struct ieee80211_dev;

/** Device operating modes. */
typedef enum {
	IEEE80211_OPMODE_ADHOC,
	IEEE80211_OPMODE_MESH,
	IEEE80211_OPMODE_AP,
	IEEE80211_OPMODE_STATION
} ieee80211_operating_mode_t;

/** IEEE 802.11 frame types. */
typedef enum {
	IEEE80211_MGMT_FRAME = 0x0,
	IEEE80211_CTRL_FRAME = 0x4,
	IEEE80211_DATA_FRAME = 0x8,
	IEEE80211_EXT_FRAME = 0xC
} ieee80211_frame_type_t;

/** IEEE 802.11 frame subtypes. */
typedef enum {
	IEEE80211_MGMT_ASSOC_REQ_FRAME = 0x00,
	IEEE80211_MGMT_ASSOC_RESP_FRAME = 0x10,
	IEEE80211_MGMT_REASSOC_REQ_FRAME = 0x20,
	IEEE80211_MGMT_REASSOC_RESP_FRAME = 0x30,
	IEEE80211_MGMT_PROBE_REQ_FRAME = 0x40,
	IEEE80211_MGMT_PROBE_RESP_FRAME = 0x50,
	IEEE80211_MGMT_BEACON_FRAME = 0x80,
	IEEE80211_MGMT_DIASSOC_FRAME = 0xA0,
	IEEE80211_MGMT_AUTH_FRAME = 0xB0,
	IEEE80211_MGMT_DEAUTH_FRAME = 0xC0,
} ieee80211_frame_subtype_t;

/** IEEE 802.11 information element types. */
typedef enum {
	IEEE80211_SSID_IE = 0,		/**< Target SSID. */
	IEEE80211_RATES_IE = 1,		/**< Supported data rates. */
	IEEE80211_CHANNEL_IE = 3,	/**< Current channel number. */
	IEEE80211_EXT_RATES_IE = 50	/**< Extended data rates. */
} ieee80211_ie_type_t;

/** IEEE 802.11 functions. */
typedef struct {
	int (*start)(struct ieee80211_dev *);
	int (*scan)(struct ieee80211_dev *);
	int (*tx_handler)(struct ieee80211_dev *, void *, size_t);
	int (*set_freq)(struct ieee80211_dev *, uint16_t);
} ieee80211_ops_t;

/** IEEE 802.11 WiFi device structure. */
typedef struct ieee80211_dev {
	/** Backing DDF device. */
	ddf_dev_t *ddf_dev;
	
	/** Pointer to implemented IEEE 802.11 operations. */
	ieee80211_ops_t *ops;
	
	/** Pointer to driver specific data. */
	void *driver_data;
	
	/** Current operating frequency. */
	uint16_t current_freq;
	
	/** Current operating mode. */
	ieee80211_operating_mode_t current_op_mode;
	
	/** BSSIDs we listen to. */
	uint8_t bssid_mask[ETH_ADDR];
	
	/* TODO: Probably to be removed later - nic.open function is now 
	 * executed multiple times, have to find out reason and fix it. 
	 */
	/** Indicates whether driver has already started. */
	bool started;
} ieee80211_dev_t;

/** IEEE 802.11 management header structure. */
typedef struct {
	uint16_t frame_ctrl;		/**< Little Endian value! */
	uint16_t duration_id;		/**< Little Endian value! */
	uint8_t dest_addr[ETH_ADDR];
	uint8_t src_addr[ETH_ADDR];
	uint8_t bssid[ETH_ADDR];
	uint16_t seq_ctrl;		/**< Little Endian value! */
} __attribute__((packed)) __attribute__ ((aligned(2))) ieee80211_mgmt_header_t;

/** IEEE 802.11 data header structure. */
typedef struct {
	uint16_t frame_ctrl;		/**< Little Endian value! */
	uint16_t duration_id;		/**< Little Endian value! */
	uint8_t address1[ETH_ADDR];
	uint8_t address2[ETH_ADDR];
	uint8_t address3[ETH_ADDR];
	uint16_t seq_ctrl;		/**< Little Endian value! */
	uint8_t address4[ETH_ADDR];
	uint16_t qos_ctrl;		/**< Little Endian value! */
} __attribute__((packed)) __attribute__ ((aligned(2))) ieee80211_data_header_t;

/** IEEE 802.11 information element header. */
typedef struct {
	uint8_t element_id;
	uint8_t length;
} __attribute__((packed)) __attribute__ ((aligned(2))) ieee80211_ie_header_t;

/** IEEE 802.11 authentication frame body. */
typedef struct {
	uint16_t auth_alg;		/**< Little Endian value! */
	uint16_t auth_trans_no;		/**< Little Endian value! */
	uint16_t status;		/**< Little Endian value! */
} __attribute__((packed)) __attribute__ ((aligned(2))) ieee80211_auth_body_t;

typedef struct {
	uint8_t bssid[ETH_ADDR];
	uint16_t auth_alg;
} __attribute__((packed)) __attribute__ ((aligned(2))) ieee80211_auth_data_t;

extern int ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, 
	void *driver_data, ddf_dev_t *ddf_dev);
extern int ieee80211_init(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops);
extern int ieee80211_probe_request(ieee80211_dev_t *ieee80211_dev);
extern int ieee80211_probe_auth(ieee80211_dev_t *ieee80211_dev);

#endif /* LIBNET_IEEE80211_H */

/** @}
 */
