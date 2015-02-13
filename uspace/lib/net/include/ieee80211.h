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

/** Initial channel frequency. */
#define IEEE80211_FIRST_CHANNEL 2412

/** Max supported channel frequency. */
#define IEEE80211_MAX_CHANNEL 2472

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

/** IEEE 802.11 functions. */
typedef struct {
	int (*start)(struct ieee80211_dev *);
	int (*scan)(struct ieee80211_dev *);
} ieee80211_ops_t;

/** IEEE 802.11 WiFi device structure. */
typedef struct ieee80211_dev {
	/** Backing DDF device. */
	ddf_dev_t *ddf_dev;
	
	/** Pointer to implemented IEEE 802.11 operations. */
	ieee80211_ops_t *ops;
	
	/** Pointer to driver specific data. */
	void *driver_data;
} ieee80211_dev_t;

extern int ieee80211_device_init(ieee80211_dev_t *ieee80211_dev, 
	void *driver_data, ddf_dev_t *ddf_dev);
extern int ieee80211_init(ieee80211_dev_t *ieee80211_dev, 
	ieee80211_ops_t *ieee80211_ops);

#endif /* LIBNET_IEEE80211_H */

/** @}
 */
