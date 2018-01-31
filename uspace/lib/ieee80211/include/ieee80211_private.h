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

/** @file ieee80211.h
 *
 * Internal IEEE 802.11 header that should not be included.
 */

#ifndef LIB_IEEE80211_PRIVATE_H
#define LIB_IEEE80211_PRIVATE_H

#include <fibril_synch.h>
#include <byteorder.h>
#include <ddf/driver.h>
#include <stddef.h>
#include <stdint.h>
#include <ieee80211/ieee80211.h>
#include "ieee80211.h"

/** Timeout in us for waiting to authentication/association response. */
#define AUTH_TIMEOUT  200000

/** Timeout in us for waiting to finish 4-way handshake process. */
#define HANDSHAKE_TIMEOUT  5000000

/** Scanning period. */
#define SCAN_PERIOD_USEC  35000000

/** Time to wait for beacons on channel. */
#define SCAN_CHANNEL_WAIT_USEC  200000

/** Max time to keep scan result. */
#define MAX_KEEP_SCAN_SPAN_SEC  120

/** Security bit in capability info field. */
#define CAP_SECURITY  0x10

/** Protocol type used in EAPOL frames. */
#define ETH_TYPE_PAE  0x888e

/** WPA OUI used in vendor specific IE. */
#define WPA_OUI  0x0050f201

/** GTK OUI used in vendor specific IE. */
#define GTK_OUI  0x000fac01

/** Max PTK key length. */
#define MAX_PTK_LENGTH  64

/** Max GTK key length. */
#define MAX_GTK_LENGTH  64

/** KEK offset inside PTK. */
#define KEK_OFFSET  16

/** TK offset inside PTK. */
#define TK_OFFSET  32

/** Length of Michael MIC code used in TKIP security suite. */
#define MIC_LENGTH  8

/** Length of data to be encrypted by PRF function.
 *
 * NONCE + SNONCE (2 * 32) + DEST_MAC + SOURCE_MAC (2 * ETH_ADDR)
 *
 */
#define PRF_CRYPT_DATA_LENGTH  (2 * 32 + 2 * ETH_ADDR)

/** Special room in header reserved for encryption. */
typedef enum {
	IEEE80211_TKIP_HEADER_LENGTH = 8,
	IEEE80211_CCMP_HEADER_LENGTH = 8
} ieee80211_encrypt_header_reserve_length_t;

/** IEEE 802.11 PTK key length. */
typedef enum {
	IEEE80211_PTK_CCMP_LENGTH = 48,
	IEEE80211_PTK_TKIP_LENGTH = 64
} ieee80211_ptk_length_t;

/** IEEE 802.11 GTK key length. */
typedef enum {
	IEEE80211_GTK_CCMP_LENGTH = 16,
	IEEE80211_GTK_TKIP_LENGTH = 32
} ieee80211_gtk_length_t;

/** IEEE 802.11 frame types. */
typedef enum {
	IEEE80211_MGMT_FRAME = 0x0,
	IEEE80211_CTRL_FRAME = 0x4,
	IEEE80211_DATA_FRAME = 0x8,
	IEEE80211_EXT_FRAME = 0xC
} ieee80211_frame_type_t;

/** IEEE 802.11 management frame subtypes. */
typedef enum {
	IEEE80211_MGMT_ASSOC_REQ_FRAME = 0x00,
	IEEE80211_MGMT_ASSOC_RESP_FRAME = 0x10,
	IEEE80211_MGMT_REASSOC_REQ_FRAME = 0x20,
	IEEE80211_MGMT_REASSOC_RESP_FRAME = 0x30,
	IEEE80211_MGMT_PROBE_REQ_FRAME = 0x40,
	IEEE80211_MGMT_PROBE_RESP_FRAME = 0x50,
	IEEE80211_MGMT_BEACON_FRAME = 0x80,
	IEEE80211_MGMT_DISASSOC_FRAME = 0xA0,
	IEEE80211_MGMT_AUTH_FRAME = 0xB0,
	IEEE80211_MGMT_DEAUTH_FRAME = 0xC0,
} ieee80211_frame_mgmt_subtype_t;

/** IEEE 802.11 data frame subtypes. */
typedef enum {
	IEEE80211_DATA_DATA_FRAME = 0x0000,
	IEEE80211_DATA_QOS_FRAME = 0x0080
} ieee80211_frame_data_subtype_t;

/** IEEE 802.11 frame control value masks. */
typedef enum {
	IEEE80211_FRAME_CTRL_FRAME_TYPE = 0x000C,
	IEEE80211_FRAME_CTRL_FRAME_SUBTYPE = 0x00F0,
	IEEE80211_FRAME_CTRL_PROTECTED = 0x4000
} ieee80211_frame_ctrl_mask_t;

/** IEEE 802.11 frame control DS field values. */
typedef enum {
	IEEE80211_FRAME_CTRL_TODS = 0x0100,
	IEEE80211_FRAME_CTRL_FROMDS = 0x0200
} ieee80211_frame_ctrl_ds_t;

/** IEEE 802.11 authentication cipher suites values. */
typedef enum {
	IEEE80211_AUTH_CIPHER_TKIP = 0x02,
	IEEE80211_AUTH_CIPHER_CCMP = 0x04
} ieee80211_auth_cipher_type_t;

/** IEEE 802.11 AKM suites values. */
typedef enum {
	IEEE80211_AUTH_AKM_8021X = 0x01,
	IEEE80211_AUTH_AKM_PSK = 0x02
} ieee80211_auth_akm_type_t;

typedef enum {
	IEEE80211_EAPOL_START = 0x1,
	IEEE80211_EAPOL_KEY = 0x3
} ieee80211_eapol_frame_type_t;

typedef enum {
	IEEE80211_EAPOL_KEY_KEYINFO_KEYTYPE = 0x0008,
	IEEE80211_EAPOL_KEY_KEYINFO_KEYID = 0x0010,
	IEEE80211_EAPOL_KEY_KEYINFO_INSTALL = 0x0040,
	IEEE80211_EAPOL_KEY_KEYINFO_ACK = 0x0080,
	IEEE80211_EAPOL_KEY_KEYINFO_MIC = 0x0100,
	IEEE80211_EAPOL_KEY_KEYINFO_SECURE = 0x0200,
	IEEE80211_EAPOL_KEY_KEYINFO_ENCDATA = 0x1000
} ieee80211_eapol_key_keyinfo_t;

/** IEEE 802.11 information element types. */
typedef enum {
	IEEE80211_SSID_IE = 0,        /**< Target SSID. */
	IEEE80211_RATES_IE = 1,       /**< Supported data rates. */
	IEEE80211_CHANNEL_IE = 3,     /**< Current channel number. */
	IEEE80211_CHALLENGE_IE = 16,  /**< Challenge text. */
	IEEE80211_RSN_IE = 48,        /**< RSN. */
	IEEE80211_EXT_RATES_IE = 50,  /**< Extended data rates. */
	IEEE80211_VENDOR_IE = 221     /**< Vendor specific IE. */
} ieee80211_ie_type_t;

/** IEEE 802.11 authentication phases. */
typedef enum {
	IEEE80211_AUTH_DISCONNECTED,
	IEEE80211_AUTH_AUTHENTICATED,
	IEEE80211_AUTH_ASSOCIATED,
	IEEE80211_AUTH_CONNECTED
} ieee80211_auth_phase_t;

/** Link with scan result info. */
typedef struct {
	link_t link;
	time_t last_beacon;
	ieee80211_scan_result_t scan_result;
	uint8_t auth_ie[256];
	size_t auth_ie_len;
} ieee80211_scan_result_link_t;

/** List of scan results info. */
typedef struct {
	list_t list;
	fibril_mutex_t results_mutex;
	size_t size;
} ieee80211_scan_result_list_t;

/** BSSID info. */
typedef struct {
	uint16_t aid;
	char password[IEEE80211_MAX_PASSW_LEN];
	uint8_t ptk[MAX_PTK_LENGTH];
	uint8_t gtk[MAX_GTK_LENGTH];
	ieee80211_scan_result_link_t *res_link;
} ieee80211_bssid_info_t;

/** IEEE 802.11 WiFi device structure. */
struct ieee80211_dev {
	/** Backing DDF device. */
	ddf_dev_t *ddf_dev;
	
	/** Pointer to implemented IEEE 802.11 device operations. */
	ieee80211_ops_t *ops;
	
	/** Pointer to implemented IEEE 802.11 interface operations. */
	ieee80211_iface_t *iface;
	
	/** Pointer to driver specific data. */
	void *specific;
	
	/** Current operating frequency. */
	uint16_t current_freq;
	
	/** Current operating mode. */
	ieee80211_operating_mode_t current_op_mode;
	
	/** Info about BSSID we are connected to. */
	ieee80211_bssid_info_t bssid_info;
	
	/**
	 * Flag indicating that data traffic is encrypted by HW key
	 * that is set up in device.
	 */
	bool using_hw_key;
	
	/** BSSIDs we listen to. */
	nic_address_t bssid_mask;
	
	/** List of APs in neighborhood. */
	ieee80211_scan_result_list_t ap_list;
	
	/** Current sequence number used in data frames. */
	uint16_t sequence_number;
	
	/** Current authentication phase. */
	ieee80211_auth_phase_t current_auth_phase;
	
	/** Flag indicating whether client wants connect to network. */
	bool pending_conn_req;
	
	/** Scanning guard. */
	fibril_mutex_t scan_mutex;
	
	/** General purpose guard. */
	fibril_mutex_t gen_mutex;
	
	/** General purpose condition variable. */
	fibril_condvar_t gen_cond;
	
	/** Indicates whether device is fully initialized. */
	bool ready;
	
	/** Indicates whether driver has already started. */
	bool started;
};

/** IEEE 802.3 (ethernet) header. */
typedef struct {
	uint8_t dest_addr[ETH_ADDR];
	uint8_t src_addr[ETH_ADDR];
	uint16_t proto;  /**< Big Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    eth_header_t;

/** IEEE 802.11 management header structure. */
typedef struct {
	uint16_t frame_ctrl;          /**< Little Endian value! */
	uint16_t duration_id;         /**< Little Endian value! */
	uint8_t dest_addr[ETH_ADDR];
	uint8_t src_addr[ETH_ADDR];
	uint8_t bssid[ETH_ADDR];
	uint16_t seq_ctrl;            /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_mgmt_header_t;

/** IEEE 802.11 data header structure. */
typedef struct {
	uint16_t frame_ctrl;         /**< Little Endian value! */
	uint16_t duration_id;        /**< Little Endian value! */
	uint8_t address1[ETH_ADDR];
	uint8_t address2[ETH_ADDR];
	uint8_t address3[ETH_ADDR];
	uint16_t seq_ctrl;           /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_data_header_t;

/** IEEE 802.11 information element header. */
typedef struct {
	uint8_t element_id;
	uint8_t length;
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_ie_header_t;

/** IEEE 802.11 authentication frame body. */
typedef struct {
	uint16_t auth_alg;       /**< Little Endian value! */
	uint16_t auth_trans_no;  /**< Little Endian value! */
	uint16_t status;         /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_auth_body_t;

/** IEEE 802.11 deauthentication frame body. */
typedef struct {
	uint16_t reason;    /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_deauth_body_t;

/** IEEE 802.11 association request frame body. */
typedef struct {
	uint16_t capability;       /**< Little Endian value! */
	uint16_t listen_interval;  /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_assoc_req_body_t;

/** IEEE 802.11 association response frame body. */
typedef struct {
	uint16_t capability;  /**< Little Endian value! */
	uint16_t status;      /**< Little Endian value! */
	uint16_t aid;         /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_assoc_resp_body_t;

/** IEEE 802.11 beacon frame body start. */
typedef struct {
	uint8_t timestamp[8];
	uint16_t beacon_interval;  /**< Little Endian value! */
	uint16_t capability;       /**< Little Endian value! */
} __attribute__((packed)) __attribute__((aligned(2)))
    ieee80211_beacon_start_t;

/** IEEE 802.11i EAPOL-Key frame format. */
typedef struct {
	uint8_t proto_version;
	uint8_t packet_type;
	uint16_t body_length;      /**< Big Endian value! */
	uint8_t descriptor_type;
	uint16_t key_info;         /**< Big Endian value! */
	uint16_t key_length;       /**< Big Endian value! */
	uint8_t key_replay_counter[8];
	uint8_t key_nonce[32];
	uint8_t eapol_key_iv[16];
	uint8_t key_rsc[8];
	uint8_t reserved[8];
	uint8_t key_mic[16];
	uint16_t key_data_length;  /**< Big Endian value! */
} __attribute__((packed)) ieee80211_eapol_key_frame_t;

#define ieee80211_scan_result_list_foreach(results, iter) \
	list_foreach((results).list, link, ieee80211_scan_result_link_t, (iter))

static inline void
    ieee80211_scan_result_list_init(ieee80211_scan_result_list_t *results)
{
	list_initialize(&results->list);
	fibril_mutex_initialize(&results->results_mutex);
}

static inline void
    ieee80211_scan_result_list_remove(ieee80211_scan_result_list_t *results,
    ieee80211_scan_result_link_t *result)
{
	list_remove(&result->link);
	results->size--;
}

static inline void
    ieee80211_scan_result_list_append(ieee80211_scan_result_list_t *results,
    ieee80211_scan_result_link_t *result)
{
	list_append(&result->link, &results->list);
	results->size++;
}

extern bool ieee80211_is_fromds_frame(uint16_t);
extern bool ieee80211_is_tods_frame(uint16_t);
extern void ieee80211_set_connect_request(ieee80211_dev_t *);
extern bool ieee80211_pending_connect_request(ieee80211_dev_t *);
extern ieee80211_auth_phase_t ieee80211_get_auth_phase(ieee80211_dev_t *);
extern void ieee80211_set_auth_phase(ieee80211_dev_t *, ieee80211_auth_phase_t);
extern errno_t ieee80211_probe_request(ieee80211_dev_t *, char *);
extern errno_t ieee80211_authenticate(ieee80211_dev_t *);
extern errno_t ieee80211_associate(ieee80211_dev_t *, char *);
extern errno_t ieee80211_deauthenticate(ieee80211_dev_t *);

#endif

/** @}
 */
