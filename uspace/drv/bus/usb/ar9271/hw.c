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

/** @file hw.c
 *
 * AR9271 hardware related functions implementation.
 *
 */

#include <usb/debug.h>
#include <unistd.h>
#include <nic.h>
#include <ieee80211.h>

#include "hw.h"
#include "wmi.h"

/**
 * Try to wait for register value repeatedly until timeout defined by device 
 * is reached.
 * 
 * @param ar9271 Device structure.
 * @param offset Registry offset (address) to be read.
 * @param mask Mask to apply on read result.
 * @param value Required value we are waiting for.
 * 
 * @return EOK if succeed, ETIMEOUT on timeout, negative error code otherwise.
 */
static int hw_read_wait(ar9271_t *ar9271, uint32_t offset, uint32_t mask,
	uint32_t value)
{
	uint32_t result;
	
	for(size_t i = 0; i < HW_WAIT_LOOPS; i++) {
		udelay(HW_WAIT_TIME_US);

		int rc = wmi_reg_read(ar9271->htc_device, offset, &result);
		if(rc != EOK) {
			usb_log_debug("Failed to read register. Error %d\n",
				rc);
			continue;
		}
		
		if((result & mask) == value) {
			return EOK;
		}
	}
	
	return ETIMEOUT;
}

static int hw_reset_power_on(ar9271_t *ar9271)
{
	wmi_reg_t buffer[] = {
		{
			.offset = AR9271_RTC_FORCE_WAKE,
			.value = AR9271_RTC_FORCE_WAKE_ENABLE | 
				AR9271_RTC_FORCE_WAKE_ON_INT
		},
		{
			.offset = AR9271_RC,
			.value = AR9271_RC_AHB
		},
		{
			.offset = AR9271_RTC_RESET,
			.value = 0
		}
	};
	
	int rc = wmi_reg_buffer_write(ar9271->htc_device, buffer,
		sizeof(buffer) / sizeof(wmi_reg_t));
	if(rc != EOK) {
		usb_log_error("Failed to set RT FORCE WAKE register.\n");
		return rc;
	}
	
	udelay(2);
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RC, 0);
	if(rc != EOK) {
		usb_log_error("Failed to reset MAC AHB register.\n");
		return rc;
	}
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RTC_RESET, 1);
	if(rc != EOK) {
		usb_log_error("Failed to bring up RTC register.\n");
		return rc;
	}
	
	rc = hw_read_wait(ar9271, 
		AR9271_RTC_STATUS, 
		AR9271_RTC_STATUS_MASK, 
		AR9271_RTC_STATUS_ON);
	if(rc != EOK) {
		usb_log_error("Failed to read wake up RTC register.\n");
		return rc;
	}
	
	return EOK;
}

static int hw_set_reset(ar9271_t *ar9271, bool cold)
{
	uint32_t reset_value = AR9271_RTC_RC_MAC_WARM;
	
	if(cold) {
		reset_value |= AR9271_RTC_RC_MAC_COLD;
	}
	
	wmi_reg_t buffer[] = {
		{
			.offset = AR9271_RTC_FORCE_WAKE,
			.value = AR9271_RTC_FORCE_WAKE_ENABLE | 
				AR9271_RTC_FORCE_WAKE_ON_INT
		},
		{
			.offset = AR9271_RC,
			.value = AR9271_RC_AHB
		},
		{
			.offset = AR9271_RTC_RC,
			.value = reset_value
		}
	};
	
	int rc = wmi_reg_buffer_write(ar9271->htc_device, buffer,
		sizeof(buffer) / sizeof(wmi_reg_t));
	if(rc != EOK) {
		usb_log_error("Failed to set warm reset register.\n");
		return rc;
	}
	
	udelay(100);
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RTC_RC, 0);
	if(rc != EOK) {
		usb_log_error("Failed to reset RTC RC register.\n");
		return rc;
	}
	
	rc = hw_read_wait(ar9271, AR9271_RTC_RC, AR9271_RTC_RC_MASK, 0);
	if(rc != EOK) {
		usb_log_error("Failed to read RTC RC register.\n");
		return rc;
	}
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RC, 0);
	if(rc != EOK) {
		usb_log_error("Failed to reset MAC AHB register.\n");
		return rc;
	}
	
	return EOK;
}

static int hw_addr_init(ar9271_t *ar9271)
{
	int rc;
	uint32_t value;
	nic_address_t ar9271_address;
	
	for(int i = 0; i < 3; i++) {
		rc = wmi_reg_read(ar9271->htc_device, 
			AR9271_EEPROM_MAC_ADDR_START + i*4,
			&value);
		
		if(rc != EOK) {
			usb_log_error("Failed to read %d. byte of MAC address."
				"\n", i);
			return rc;
		}
		
		uint16_t two_bytes = uint16_t_be2host(value);
		ar9271_address.address[2*i] = two_bytes >> 8;
		ar9271_address.address[2*i+1] = two_bytes & 0xff;
	}
	
	nic_t *nic = nic_get_from_ddf_dev(ar9271->ddf_dev);
	
	rc = nic_report_address(nic, &ar9271_address);
	if(rc != EOK) {
		usb_log_error("Failed to report NIC HW address.\n");
			return rc;
	}
	
	return EOK;
}

static int hw_gpio_set_output(ar9271_t *ar9271, uint32_t gpio, uint32_t type)
{
	uint32_t address, gpio_shift, temp;
	
	if(gpio > 11) {
		address = AR9271_GPIO_OUT_MUX3;
	} else if(gpio > 5) {
		address = AR9271_GPIO_OUT_MUX2;
	} else {
		address = AR9271_GPIO_OUT_MUX1;
	}
	
	gpio_shift = (gpio % 6) * 5;
	
	int rc = wmi_reg_read(ar9271->htc_device, address, &temp);
	if(rc != EOK) {
		usb_log_error("Failed to read GPIO output mux.\n");
		return rc;
	}
	
	temp = ((temp & 0x1F0) << 1) | (temp & ~0x1F0);
	temp &= ~(0x1f << gpio_shift);
	temp |= (type << gpio_shift);
	
	rc = wmi_reg_write(ar9271->htc_device, address, temp);
	if(rc != EOK) {
		usb_log_error("Failed to write GPIO output mux.\n");
		return rc;
	}
	
	gpio_shift = 2 * gpio;
	
	rc = wmi_reg_clear_set_bit(ar9271->htc_device, AR9271_GPIO_OE_OUT,
		AR9271_GPIO_OE_OUT_ALWAYS << gpio_shift, 
		AR9271_GPIO_OE_OUT_ALWAYS << gpio_shift);
	if(rc != EOK) {
		usb_log_error("Failed to config GPIO as output.\n");
		return rc;
	}
	
	return EOK;
}

static int hw_gpio_set_value(ar9271_t *ar9271, uint32_t gpio, uint32_t value)
{
	int rc = wmi_reg_clear_set_bit(ar9271->htc_device, AR9271_GPIO_IN_OUT,
		(~value & 1) << gpio, 1 << gpio);
	if(rc != EOK) {
		usb_log_error("Failed to set GPIO.\n");
		return rc;
	}
	
	return EOK;
}

/**
 * Hardware init procedure of AR9271 device.
 * 
 * @param ar9271 Device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
static int hw_init_proc(ar9271_t *ar9271)
{
	int rc = hw_reset_power_on(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to HW reset power on.\n");
		return rc;
	}
	
	rc = hw_set_reset(ar9271, false);
	if(rc != EOK) {
		usb_log_error("Failed to HW warm reset.\n");
		return rc;
	}
	
	rc = hw_addr_init(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to init HW addr.\n");
		return rc;
	}
	
	return EOK;
}

static int hw_init_led(ar9271_t *ar9271)
{
	int rc = hw_gpio_set_output(ar9271, AR9271_LED_PIN, 
		AR9271_GPIO_OUT_MUX_AS_OUT);
	if(rc != EOK) {
		usb_log_error("Failed to set led GPIO to output.\n");
		return rc;
	}
	
	rc = hw_gpio_set_value(ar9271, AR9271_LED_PIN, 0);
	if(rc != EOK) {
		usb_log_error("Failed to init bring up GPIO led.\n");
		return rc;
	}
	
	return EOK;
}

static int hw_set_operating_mode(ar9271_t *ar9271, 
	ieee80211_operating_mode_t opmode)
{
	uint32_t set_bit = 0x10000000;
	
	/* NOTICE: Fall-through switch statement! */
	switch(opmode) {
		case IEEE80211_OPMODE_ADHOC:
			set_bit |= AR9271_OPMODE_ADHOC_MASK;
		case IEEE80211_OPMODE_MESH:
		case IEEE80211_OPMODE_AP:
			set_bit |= AR9271_OPMODE_STATION_AP_MASK;
		case IEEE80211_OPMODE_STATION:
			wmi_reg_clear_bit(ar9271->htc_device, AR9271_CONFIG,
				AR9271_CONFIG_ADHOC);
	}
	
	wmi_reg_clear_set_bit(ar9271->htc_device, AR9271_STATION_ID1,
		set_bit, 
		AR9271_OPMODE_STATION_AP_MASK | AR9271_OPMODE_ADHOC_MASK);
	
	return EOK;
}

static uint32_t hw_reverse_bits(uint32_t value, uint32_t count)
{
	uint32_t ret_val = 0;
	
	for(size_t i = 0; i < count; i++) {
		ret_val = (ret_val << 1) | (value & 1);
		value >>= 1;
	}
	
	return ret_val;
}

static int hw_set_channel(ar9271_t *ar9271, uint16_t freq)
{
	/* Not supported channel frequency. */
	if(freq < IEEE80211_FIRST_CHANNEL || freq > IEEE80211_MAX_CHANNEL) {
		return EINVAL;
	}
	
	/* Not supported channel frequency. */
	if((freq - IEEE80211_FIRST_CHANNEL) % IEEE80211_CHANNEL_GAP != 0) {
		return EINVAL;
	}
	
	uint32_t result;
	wmi_reg_read(ar9271->htc_device, AR9271_PHY_CCK_TX_CTRL, &result);
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_CCK_TX_CTRL,
		result & ~AR9271_PHY_CCK_TX_CTRL_JAPAN);
	
	/* Some magic here. */
	uint32_t channel = hw_reverse_bits(
		(((((freq - 672) * 2 - 3040) / 10) << 2) & 0xFF), 8);
	uint32_t to_write = (channel << 8) | 33;
	
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_BASE + (0x37 << 2),
		to_write);
	
	return EOK;
}

int hw_reset(ar9271_t *ar9271) 
{
	int rc = wmi_reg_write(ar9271->htc_device, 
		AR9271_RESET_POWER_DOWN_CONTROL,
		AR9271_RADIO_RF_RESET);
	if(rc != EOK) {
		usb_log_error("Failed to reset radio rf.\n");
		return rc;
	}
	
	udelay(50);
	
	rc = wmi_reg_write(ar9271->htc_device, 
		AR9271_RESET_POWER_DOWN_CONTROL,
		AR9271_GATE_MAC_CONTROL);
	if(rc != EOK) {
		usb_log_error("Failed to set the gate reset controls for MAC."
			"\n");
		return rc;
	}
	
	udelay(50);
	
	/* Perform cold reset of device. */
	rc = hw_set_reset(ar9271, true);
	if(rc != EOK) {
		usb_log_error("Failed to HW cold reset.\n");
		return rc;
	}
	
	/* Set physical layer mode. */
	rc = wmi_reg_write(ar9271->htc_device, AR9271_PHY_MODE,
		AR9271_PHY_MODE_DYNAMIC | AR9271_PHY_MODE_2G);
	if(rc != EOK) {
		usb_log_error("Failed to set physical layer mode.\n");
		return rc;
	}
	
	/* Set device operating mode. */
	rc = hw_set_operating_mode(ar9271, IEEE80211_OPMODE_STATION);
	if(rc != EOK) {
		usb_log_error("Failed to set opmode to station.\n");
		return rc;
	}
	
	/* Set channel. */
	rc = hw_set_channel(ar9271, IEEE80211_FIRST_CHANNEL);
	if(rc != EOK) {
		usb_log_error("Failed to set channel.\n");
		return rc;
	}
	
	/* Initialize transmission queues. */
	for(int i = 0; i < AR9271_QUEUES_COUNT; i++) {
		rc = wmi_reg_write(ar9271->htc_device, 
			AR9271_QUEUE_BASE_MASK + (i << 2),
			1 << i);
		if(rc != EOK) {
			usb_log_error("Failed to initialize transmission queue."
				"\n");
			return rc;
		}
	}
	
	/* Activate physical layer. */
	rc = wmi_reg_write(ar9271->htc_device, AR9271_PHY_ACTIVE, 1);
	if(rc != EOK) {
		usb_log_error("Failed to activate physical layer.\n");
		return rc;
	}
	
	return EOK;
}

/**
 * Initialize hardware of AR9271 device.
 * 
 * @param ar9271 Device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int hw_init(ar9271_t *ar9271)
{
	int rc = hw_init_proc(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to HW reset device.\n");
		return rc;
	}
	
	rc = hw_init_led(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to HW init led.\n");
		return rc;
	}
	
	usb_log_info("HW initialization finished successfully.\n");
	
	return EOK;
}