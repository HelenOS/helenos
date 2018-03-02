/*
 * Copyright (c) 2014 Jan Kolarik
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

/** @file htc.h
 *
 * Definitions of the Atheros HTC (Host Target Communication) technology
 * for communication between host (PC) and target (device firmware).
 *
 */

#ifndef ATHEROS_HTC_H
#define ATHEROS_HTC_H

#include <ieee80211.h>
#include <fibril_synch.h>
#include <usb/dev/driver.h>
#include <stddef.h>
#include <stdint.h>
#include <nic.h>
#include "ath.h"

#define HTC_RTS_THRESHOLD     2304
#define HTC_RATES_MAX_LENGTH  30

/** HTC message IDs.
 *
 */
typedef enum {
	HTC_MESSAGE_READY = 1,
	HTC_MESSAGE_CONNECT_SERVICE,
	HTC_MESSAGE_CONNECT_SERVICE_RESPONSE,
	HTC_MESSAGE_SETUP_COMPLETE,
	HTC_MESSAGE_CONFIG
} htc_message_id_t;

/** HTC response message status codes.
 *
 */
typedef enum {
	HTC_SERVICE_SUCCESS = 0,
	HTC_SERVICE_NOT_FOUND,
	HTC_SERVICE_FAILED,
	HTC_SERVICE_NO_RESOURCES,
	HTC_SERVICE_NO_MORE_EP
} htc_response_status_code_t;

/** HTC operating mode definition.
 *
 */
typedef enum {
	HTC_OPMODE_ADHOC = 0,
	HTC_OPMODE_STATION = 1,
	HTC_OPMODE_MESH = 2,
	HTC_OPMODE_AP = 6
} htc_operating_mode_t;

/** HTC data type indicator.
 *
 */
typedef enum {
	HTC_DATA_AMPDU = 1,
	HTC_DATA_NORMAL = 2,
	HTC_DATA_BEACON = 3,
	HTC_DATA_MGMT = 4
} htc_data_type_t;

/** HTC endpoint numbers.
 *
 */
typedef struct {
	int ctrl_endpoint;
	int wmi_endpoint;
	int beacon_endpoint;
	int cab_endpoint;
	int uapsd_endpoint;
	int mgmt_endpoint;
	int data_be_endpoint;
	int data_bk_endpoint;
	int data_video_endpoint;
	int data_voice_endpoint;
} htc_pipes_t;

/** HTC device data.
 *
 */
typedef struct {
	/** WMI message sequence number */
	uint16_t sequence_number;

	/** HTC endpoints numbers */
	htc_pipes_t endpoints;

	/** Lock for receiver */
	fibril_mutex_t rx_lock;

	/** Lock for transmitter */
	fibril_mutex_t tx_lock;

	/** Pointer to related IEEE 802.11 device */
	ieee80211_dev_t *ieee80211_dev;

	/** Pointer to Atheros WiFi device structure */
	ath_t *ath_device;
} htc_device_t;

/** HTC frame header structure.
 *
 */
typedef struct {
	uint8_t endpoint_id;
	uint8_t flags;
	uint16_t payload_length;   /**< Big Endian value! */
	uint8_t control_bytes[4];
} __attribute__((packed)) htc_frame_header_t;

/** HTC management TX frame header structure.
 *
 */
typedef struct {
	uint8_t node_idx;
	uint8_t vif_idx;
	uint8_t tidno;
	uint8_t flags;
	uint8_t key_type;
	uint8_t keyix;
	uint8_t cookie;
	uint8_t pad;
} __attribute__((packed)) htc_tx_management_header_t;

/** HTC data TX frame header structure.
 *
 */
typedef struct {
	uint8_t data_type;
	uint8_t node_idx;
	uint8_t vif_idx;
	uint8_t tidno;
	uint32_t flags;    /**< Big Endian value! */
	uint8_t key_type;
	uint8_t keyix;
	uint8_t cookie;
	uint8_t pad;
} __attribute__((packed)) htc_tx_data_header_t;

/** HTC ready message structure.
 *
 */
typedef struct {
	uint16_t message_id;   /**< Big Endian value! */
	uint16_t credits;      /**< Big Endian value! */
	uint16_t credit_size;  /**< Big Endian value! */

	uint8_t max_endpoints;
	uint8_t pad;
} __attribute__((packed)) htc_ready_msg_t;

/** HTC service message structure.
 *
 */
typedef struct {
	uint16_t message_id;        /**< Big Endian value! */
	uint16_t service_id;        /**< Big Endian value! */
	uint16_t connection_flags;  /**< Big Endian value! */

	uint8_t download_pipe_id;
	uint8_t upload_pipe_id;
	uint8_t service_meta_length;
	uint8_t pad;
} __attribute__((packed)) htc_service_msg_t;

/** HTC service response message structure.
 *
 */
typedef struct {
	uint16_t message_id;          /**< Big Endian value! */
	uint16_t service_id;          /**< Big Endian value! */
	uint8_t status;
	uint8_t endpoint_id;
	uint16_t max_message_length;  /**< Big Endian value! */
	uint8_t service_meta_length;
	uint8_t pad;
} __attribute__((packed)) htc_service_resp_msg_t;

/** HTC credits config message structure.
 *
 */
typedef struct {
	uint16_t message_id;  /**< Big Endian value! */
	uint8_t pipe_id;
	uint8_t credits;
} __attribute__((packed)) htc_config_msg_t;

/** HTC new virtual interface message.
 *
 */
typedef struct {
	uint8_t index;
	uint8_t op_mode;
	uint8_t addr[ETH_ADDR];
	uint8_t ath_cap;
	uint16_t rts_thres;      /**< Big Endian value! */
	uint8_t pad;
} __attribute__((packed)) htc_vif_msg_t;

/** HTC new station message.
 *
 */
typedef struct {
	uint8_t addr[ETH_ADDR];
	uint8_t bssid[ETH_ADDR];
	uint8_t sta_index;
	uint8_t vif_index;
	uint8_t is_vif_sta;

	uint16_t flags;      /**< Big Endian value! */
	uint16_t ht_cap;     /**< Big Endian value! */
	uint16_t max_ampdu;  /**< Big Endian value! */

	uint8_t pad;
} __attribute__((packed)) htc_sta_msg_t;

/** HTC message to inform target about available capabilities.
 *
 */
typedef struct {
	uint32_t ampdu_limit;     /**< Big Endian value! */
	uint8_t ampdu_subframes;
	uint8_t enable_coex;
	uint8_t tx_chainmask;
	uint8_t pad;
} __attribute__((packed)) htc_cap_msg_t;

typedef struct {
	uint8_t sta_index;
	uint8_t is_new;
	uint32_t cap_flags;  /**< Big Endian value! */
	uint8_t legacy_rates_count;
	uint8_t legacy_rates[HTC_RATES_MAX_LENGTH];
	uint16_t pad;
} htc_rate_msg_t;

/** HTC RX status structure used in incoming HTC data messages.
 *
 */
typedef struct {
	uint64_t timestamp;    /**< Big Endian value! */
	uint16_t data_length;  /**< Big Endian value! */
	uint8_t status;
	uint8_t phy_err;
	int8_t rssi;
	int8_t rssi_ctl[3];
	int8_t rssi_ext[3];
	uint8_t keyix;
	uint8_t rate;
	uint8_t antenna;
	uint8_t more;
	uint8_t is_aggr;
	uint8_t more_aggr;
	uint8_t num_delims;
	uint8_t flags;
	uint8_t dummy;
	uint32_t evm0;         /**< Big Endian value! */
	uint32_t evm1;         /**< Big Endian value! */
	uint32_t evm2;         /**< Big Endian value! */
} htc_rx_status_t;

/** HTC setup complete message structure
 *
 */
typedef struct {
	uint16_t message_id;  /**< Big Endian value! */
} __attribute__((packed)) htc_setup_complete_msg_t;

extern errno_t htc_device_init(ath_t *, ieee80211_dev_t *, htc_device_t *);
extern errno_t htc_init(htc_device_t *);
extern errno_t htc_init_new_vif(htc_device_t *);
extern errno_t htc_read_control_message(htc_device_t *, void *, size_t, size_t *);
extern errno_t htc_read_data_message(htc_device_t *, void *, size_t, size_t *);
extern errno_t htc_send_control_message(htc_device_t *, void *, size_t, uint8_t);
extern errno_t htc_send_data_message(htc_device_t *, void *, size_t, uint8_t);

#endif  /* ATHEROS_HTC_H */
