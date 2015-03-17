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

#define DEVICE_CATEGORY_IEEE80211 "ieee80211"

struct ieee80211_dev;
typedef struct ieee80211_dev ieee80211_dev_t;

/** Initial channel frequency. */
#define IEEE80211_FIRST_FREQ 2412

/** Max supported channel frequency. */
#define IEEE80211_MAX_FREQ 2472

/* Gap between IEEE80211 channels in MHz. */
#define IEEE80211_CHANNEL_GAP 5

/** Device operating modes. */
typedef enum {
	IEEE80211_OPMODE_ADHOC,
	IEEE80211_OPMODE_MESH,
	IEEE80211_OPMODE_AP,
	IEEE80211_OPMODE_STATION
} ieee80211_operating_mode_t;

/** IEEE 802.11 callback functions. */
typedef struct {
	/**
	 * Function that is called at device initalization. This should
	 * get device into running state.
	 * 
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * 
	 * @return EOK if succeed, negative error code otherwise.
	 */
	int (*start)(struct ieee80211_dev *);
	
	/**
	 * Scan neighborhood for networks. There should be implemented
	 * scanning of whole bandwidth. Incoming results are processed by
	 * IEEE 802.11 framework itself.
	 * 
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * 
	 * @return EOK if succeed, negative error code otherwise.
	 */
	int (*scan)(struct ieee80211_dev *);
	
	/**
	 * Handler for TX frames to be send from device. This should be
	 * called for every frame that has to be send from IEEE 802.11 device.
	 * 
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param buffer Buffer with data to be send.
	 * @param buffer_size Size of buffer.
	 * 
	 * @return EOK if succeed, negative error code otherwise.
	 */
	int (*tx_handler)(struct ieee80211_dev *, void *, size_t);
	
	/**
	 * Set device operating frequency to given value.
	 * 
	 * @param ieee80211_dev Pointer to IEEE 802.11 device structure.
	 * @param freq New device operating frequency.
	 * 
	 * @return EOK if succeed, negative error code otherwise.
	 */
	int (*set_freq)(struct ieee80211_dev *, uint16_t);
} ieee80211_ops_t;

/* Initialization functions. */
extern ieee80211_dev_t *ieee80211_device_create(void);
extern int ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, 
	ddf_dev_t *ddf_dev);
extern int ieee80211_init(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops, ieee80211_iface_t *ieee80211_iface,
	nic_iface_t *ieee80211_nic_iface, ddf_dev_ops_t *ieee80211_dev_ops);

/* Getters & setters, queries & reports. */
extern void *ieee80211_get_specific(ieee80211_dev_t *ieee80211_dev);
extern void ieee80211_set_specific(ieee80211_dev_t *ieee80211_dev, 
	void *specific);
extern ddf_dev_t *ieee80211_get_ddf_dev(ieee80211_dev_t* ieee80211_dev);
extern ieee80211_operating_mode_t ieee80211_query_current_op_mode(ieee80211_dev_t *ieee80211_dev);
extern uint16_t ieee80211_query_current_freq(ieee80211_dev_t *ieee80211_dev);
extern void ieee80211_report_current_op_mode(ieee80211_dev_t *ieee80211_dev,
	ieee80211_operating_mode_t op_mode);
extern void ieee80211_report_current_freq(ieee80211_dev_t *ieee80211_dev,
	uint16_t freq);

extern bool ieee80211_is_data_frame(uint16_t frame_ctrl);
extern bool ieee80211_is_mgmt_frame(uint16_t frame_ctrl);
extern bool ieee80211_is_beacon_frame(uint16_t frame_ctrl);
extern bool ieee80211_is_probe_response_frame(uint16_t frame_ctrl);

/* Worker functions. */
extern int ieee80211_rx_handler(ieee80211_dev_t *ieee80211_dev, void *buffer,
	size_t buffer_size);

#endif // LIB_IEEE80211_H

/** @}
 */
