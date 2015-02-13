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

/** @file ar9271.h
 *
 * Header file for AR9271 USB WiFi dongle.
 *
 */

#ifndef AR9271_H_
#define AR9271_H_

#include <usb/dev/driver.h>

#include "htc.h"

/** Max supported channel frequency. */
#define AR9271_MAX_CHANNEL 2472

/** Number of transmisson queues */
#define AR9271_QUEUES_COUNT 10

/** Number of GPIO pin used for handling led light */
#define AR9271_LED_PIN 15

/** AR9271 Registers */
typedef enum {
	/* ATH command register */
	AR9271_COMMAND = 0x0008,
	AR9271_COMMAND_RX_ENABLE = 0x00000004,
	
	/* ATH config register */
	AR9271_CONFIG = 0x0014,
	AR9271_CONFIG_ADHOC = 0x00000020,
	
	AR9271_QUEUE_BASE_MASK = 0x1000,
	
	/* EEPROM Addresses */
	AR9271_EEPROM_BASE = 0x2100,
	AR9271_EEPROM_MAC_ADDR_START = 0x2118,
	
	/* Reset MAC interface */
	AR9271_RC = 0x4000,
	AR9271_RC_AHB = 0x00000001,
		
	/* GPIO registers */
	AR9271_GPIO_IN_OUT = 0x4048,		/**< GPIO value read/set  */
	AR9271_GPIO_OE_OUT = 0x404C,		/**< GPIO set to output  */
	AR9271_GPIO_OE_OUT_ALWAYS = 0x3,	/**< GPIO always drive output */
	AR9271_GPIO_OUT_MUX1 = 0x4060,
	AR9271_GPIO_OUT_MUX2 = 0x4064,
	AR9271_GPIO_OUT_MUX3 = 0x4068,
	AR9271_GPIO_OUT_MUX_AS_OUT = 0x0,	/**< GPIO set mux as output */
    
	/* Wakeup related registers */
	AR9271_RTC_RC = 0x7000,
	AR9271_RTC_RC_MAC_WARM = 0x00000001,
	AR9271_RTC_RC_MAC_COLD = 0x00000002,
	AR9271_RTC_RC_MASK = 0x00000003,
	AR9271_RTC_RESET = 0x7040,
	AR9271_RTC_STATUS = 0x7044,
	AR9271_RTC_STATUS_MASK = 0x0000000F,
	AR9271_RTC_STATUS_SHUTDOWN = 0x00000001,
	AR9271_RTC_STATUS_ON = 0x00000002,
	AR9271_RTC_FORCE_WAKE = 0x704C,
	AR9271_RTC_FORCE_WAKE_ENABLE = 0x00000001,
	AR9271_RTC_FORCE_WAKE_ON_INT = 0x00000002,
		
	AR9271_RX_FILTER = 0x803C,
	AR9271_RX_FILTER_UNI = 0x00000001,
	AR9271_RX_FILTER_MULTI = 0x00000002,
	AR9271_RX_FILTER_BROAD = 0x00000004,
	AR9271_RX_FILTER_CONTROL = 0x00000008,
	AR9271_RX_FILTER_BEACON = 0x00000010,
	AR9271_RX_FILTER_PROMISCUOUS = 0x00000020,
	AR9271_RX_FILTER_PROBEREQ = 0x00000080,
		
	AR9271_PHY_BASE = 0x9800,
	AR9271_PHY_ACTIVE = 0x981C,	
	AR9271_PHY_MODE = 0xA200,
	AR9271_PHY_MODE_2G = 0x02,
	AR9271_PHY_MODE_DYNAMIC = 0x04,
	AR9271_PHY_CCK_TX_CTRL = 0xA204,
	AR9271_PHY_CCK_TX_CTRL_JAPAN = 0x00000010,
		
	AR9271_OPMODE_STATION_AP_MASK =	0x00010000,
	AR9271_OPMODE_ADHOC_MASK = 0x00020000,
		
	AR9271_RESET_POWER_DOWN_CONTROL = 0x50044,
	AR9271_RADIO_RF_RESET = 0x20,
	AR9271_GATE_MAC_CONTROL = 0x4000,
    
	/* FW Addresses */
	AR9271_FW_ADDRESS =	0x501000,
	AR9271_FW_OFFSET =	0x903000,
	
	/* MAC Registers */
	AR9271_STATION_ID0 = 0x8000, /**< STA Address Lower 32 Bits */
	AR9271_STATION_ID1 = 0x8004, /**< STA Address Upper 16 Bits */
	AR9271_STATION_BSSID0 = 0x8008, /**< BSSID Lower 32 Bits */
	AR9271_STATION_BSSID1 = 0x800C, /**< BSSID Upper 16 Bits */
} ar9271_registers_t;

/** AR9271 Requests */
typedef enum {
	AR9271_FW_DOWNLOAD = 0x30,
	AR9271_FW_DOWNLOAD_COMP = 0x31,
} ar9271_requests_t;

/** AR9271 device data */
typedef struct {
	/** Backing DDF device */
	ddf_dev_t *ddf_dev;
	
	/** USB device data */
	usb_device_t *usb_device;
	
	/** IEEE 802.11 device data */
	ieee80211_dev_t *ieee80211_dev;
	
	/** ATH device data */
	ath_t *ath_device;
	
	/** HTC device data */
	htc_device_t *htc_device;
} ar9271_t;

#endif