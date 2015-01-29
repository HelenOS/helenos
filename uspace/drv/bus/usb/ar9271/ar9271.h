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

/** Number of GPIO pin used for handling led light */
#define AR9271_LED_PIN 15

/** AR9271 Registers */
typedef enum {
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
	AR9271_RTC_RC_MASK = 0x00000003,
	AR9271_RTC_RESET = 0x7040,
	AR9271_RTC_STATUS = 0x7044,
	AR9271_RTC_STATUS_MASK = 0x0000000F,
	AR9271_RTC_STATUS_ON = 0x00000002,
	AR9271_RTC_FORCE_WAKE = 0x704C,
	AR9271_RTC_FORCE_WAKE_ENABLE = 0x00000001,
	AR9271_RTC_FORCE_WAKE_ON_INT = 0x00000002,
    
	/* FW Addresses */
	AR9271_FW_ADDRESS =	0x501000,
	AR9271_FW_OFFSET =	0x903000,
	
	/* MAC Registers */
	AR9271_MAC_PCU_STA_ADDR_L32 = 0x8000, /**< STA Address Lower 32 Bits */
	AR9271_MAC_PCU_STA_ADDR_U16 = 0x8004, /**< STA Address Upper 16 Bits */
	AR9271_MAC_PCU_BSSID_L32 = 0x8008, /**< BSSID Lower 32 Bits */
	AR9271_MAC_PCU_BSSID_U16 = 0x800C, /**< BSSID Upper 16 Bits */
} ar9271_registers_t;

/** AR9271 Requests */
typedef enum {
	AR9271_FW_DOWNLOAD = 0x30,
	AR9271_FW_DOWNLOAD_COMP = 0x31,
} ar9271_requests_t;

/** AR9271 device data */
typedef struct {
	/** DDF device pointer */
	ddf_dev_t *ddf_device;
	
	/** USB device data */
	usb_device_t *usb_device;
	
	/** HTC device data */
	htc_device_t *htc_device;
} ar9271_t;

#endif