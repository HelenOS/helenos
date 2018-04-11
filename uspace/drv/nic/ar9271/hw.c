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

#include <macros.h>
#include <usb/debug.h>
#include <errno.h>
#include <nic.h>
#include <ieee80211.h>
#include "hw.h"
#include "wmi.h"

/** Try to wait for register value repeatedly until timeout is reached.
 *
 * @param ar9271 Device structure.
 * @param offset Registry offset (address) to be read.
 * @param mask   Mask to apply on read result.
 * @param value  Required value we are waiting for.
 *
 * @return EOK if succeed, ETIMEOUT on timeout,
 *         error code otherwise.
 *
 */
static errno_t hw_read_wait(ar9271_t *ar9271, uint32_t offset, uint32_t mask,
    uint32_t value)
{
	for (size_t i = 0; i < HW_WAIT_LOOPS; i++) {
		udelay(HW_WAIT_TIME_US);

		uint32_t result;
		wmi_reg_read(ar9271->htc_device, offset, &result);
		if ((result & mask) == value)
			return EOK;
	}

	return ETIMEOUT;
}

static errno_t hw_reset_power_on(ar9271_t *ar9271)
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

	wmi_reg_buffer_write(ar9271->htc_device, buffer,
	    sizeof(buffer) / sizeof(wmi_reg_t));

	udelay(2);

	wmi_reg_write(ar9271->htc_device, AR9271_RC, 0);
	wmi_reg_write(ar9271->htc_device, AR9271_RTC_RESET, 1);

	errno_t rc = hw_read_wait(ar9271,
	    AR9271_RTC_STATUS,
	    AR9271_RTC_STATUS_MASK,
	    AR9271_RTC_STATUS_ON);
	if (rc != EOK) {
		usb_log_error("Failed to wait for RTC wake up register.\n");
		return rc;
	}

	return EOK;
}

static errno_t hw_set_reset(ar9271_t *ar9271, bool cold)
{
	uint32_t reset_value = AR9271_RTC_RC_MAC_WARM;

	if (cold)
		reset_value |= AR9271_RTC_RC_MAC_COLD;

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

	wmi_reg_buffer_write(ar9271->htc_device, buffer,
	    sizeof(buffer) / sizeof(wmi_reg_t));

	udelay(100);

	wmi_reg_write(ar9271->htc_device, AR9271_RTC_RC, 0);

	errno_t rc = hw_read_wait(ar9271, AR9271_RTC_RC, AR9271_RTC_RC_MASK, 0);
	if (rc != EOK) {
		usb_log_error("Failed to wait for RTC RC register.\n");
		return rc;
	}

	wmi_reg_write(ar9271->htc_device, AR9271_RC, 0);
	wmi_reg_clear_bit(ar9271->htc_device, AR9271_STATION_ID1,
	    AR9271_STATION_ID1_POWER_SAVING);

	return EOK;
}

static errno_t hw_addr_init(ar9271_t *ar9271)
{
	uint32_t value;
	nic_address_t ar9271_address;

	for (unsigned int i = 0; i < 3; i++) {
		wmi_reg_read(ar9271->htc_device,
		    AR9271_EEPROM_MAC_ADDR_START + i * 4, &value);

		uint16_t two_bytes = uint16_t_be2host(value);
		ar9271_address.address[2 * i] = two_bytes >> 8;
		ar9271_address.address[2 * i + 1] = two_bytes & 0xff;
	}

	nic_t *nic = nic_get_from_ddf_dev(ar9271->ddf_dev);

	errno_t rc = nic_report_address(nic, &ar9271_address);
	if (rc != EOK) {
		usb_log_error("Failed to report NIC HW address.\n");
		return rc;
	}

	return EOK;
}

static errno_t hw_gpio_set_output(ar9271_t *ar9271, uint32_t gpio, uint32_t type)
{
	uint32_t address;

	if (gpio > 11)
		address = AR9271_GPIO_OUT_MUX3;
	else if (gpio > 5)
		address = AR9271_GPIO_OUT_MUX2;
	else
		address = AR9271_GPIO_OUT_MUX1;

	uint32_t gpio_shift = (gpio % 6) * 5;

	uint32_t temp;
	wmi_reg_read(ar9271->htc_device, address, &temp);

	temp = ((temp & 0x1f0) << 1) | (temp & ~0x1f0);
	temp &= ~(0x1f << gpio_shift);
	temp |= (type << gpio_shift);

	wmi_reg_write(ar9271->htc_device, address, temp);

	gpio_shift = 2 * gpio;

	wmi_reg_set_clear_bit(ar9271->htc_device, AR9271_GPIO_OE_OUT,
	    AR9271_GPIO_OE_OUT_ALWAYS << gpio_shift,
	    AR9271_GPIO_OE_OUT_ALWAYS << gpio_shift);

	return EOK;
}

static errno_t hw_gpio_set_value(ar9271_t *ar9271, uint32_t gpio, uint32_t value)
{
	wmi_reg_set_clear_bit(ar9271->htc_device, AR9271_GPIO_IN_OUT,
	    (~value & 1) << gpio, 1 << gpio);
	return EOK;
}

/**Hardware init procedure of AR9271 device.
 *
 * @param ar9271 Device structure.
 *
 * @return EOK if succeed, error code otherwise.
 *
 */
static errno_t hw_init_proc(ar9271_t *ar9271)
{
	errno_t rc = hw_reset_power_on(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to HW reset power on.\n");
		return rc;
	}

	rc = hw_set_reset(ar9271, false);
	if (rc != EOK) {
		usb_log_error("Failed to HW warm reset.\n");
		return rc;
	}

	rc = hw_addr_init(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to init HW addr.\n");
		return rc;
	}

	return EOK;
}

static errno_t hw_init_led(ar9271_t *ar9271)
{
	errno_t rc = hw_gpio_set_output(ar9271, AR9271_LED_PIN,
	    AR9271_GPIO_OUT_MUX_AS_OUT);
	if (rc != EOK) {
		usb_log_error("Failed to set led GPIO to output.\n");
		return rc;
	}

	rc = hw_gpio_set_value(ar9271, AR9271_LED_PIN, 0);
	if (rc != EOK) {
		usb_log_error("Failed to init bring up GPIO led.\n");
		return rc;
	}

	return EOK;
}

static errno_t hw_activate_phy(ar9271_t *ar9271)
{
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_ACTIVE, 1);
	udelay(1000);

	return EOK;
}

static errno_t hw_set_operating_mode(ar9271_t *ar9271,
    ieee80211_operating_mode_t op_mode)
{
	uint32_t set_bit = 0x10000000;

	switch (op_mode) {
	case IEEE80211_OPMODE_ADHOC:
		set_bit |= AR9271_OPMODE_ADHOC_MASK;
		wmi_reg_set_bit(ar9271->htc_device, AR9271_CONFIG,
		    AR9271_CONFIG_ADHOC);
		break;
	case IEEE80211_OPMODE_MESH:
	case IEEE80211_OPMODE_AP:
		set_bit |= AR9271_OPMODE_STATION_AP_MASK;
		/* Fallthrough */
	case IEEE80211_OPMODE_STATION:
		wmi_reg_clear_bit(ar9271->htc_device, AR9271_CONFIG,
		    AR9271_CONFIG_ADHOC);
	}

	wmi_reg_set_clear_bit(ar9271->htc_device, AR9271_STATION_ID1,
	    set_bit, AR9271_OPMODE_STATION_AP_MASK | AR9271_OPMODE_ADHOC_MASK);

	ieee80211_report_current_op_mode(ar9271->ieee80211_dev, op_mode);

	return EOK;
}

static errno_t hw_reset_operating_mode(ar9271_t *ar9271)
{
	errno_t rc = hw_set_operating_mode(ar9271, IEEE80211_OPMODE_STATION);
	if (rc != EOK) {
		usb_log_error("Failed to set opmode to station.\n");
		return rc;
	}

	return EOK;
}

static errno_t hw_noise_floor_calibration(ar9271_t *ar9271)
{
	uint32_t value;
	wmi_reg_read(ar9271->htc_device, AR9271_PHY_CAL, &value);

	value &= 0xfffffe00;
	value |= (((uint32_t) AR9271_CALIB_NOMINAL_VALUE_2GHZ << 1) & 0x1ff);

	wmi_reg_write(ar9271->htc_device, AR9271_PHY_CAL, value);

	wmi_reg_clear_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_CALIB_EN);

	wmi_reg_clear_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_NOT_UPDATE);

	wmi_reg_set_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_CALIB);

	errno_t rc = hw_read_wait(ar9271, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_CALIB, 0);
	if (rc != EOK) {
		usb_log_error("Failed to wait for NF calibration.\n");
		return rc;
	}

	wmi_reg_set_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_CALIB_EN);

	wmi_reg_clear_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_NOT_UPDATE);

	wmi_reg_set_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_NF_CALIB);

	return EOK;
}

static errno_t hw_set_freq(ar9271_t *ar9271, uint16_t freq)
{
	/* Not supported channel frequency. */
	if ((freq < IEEE80211_FIRST_FREQ) || (freq > IEEE80211_MAX_FREQ))
		return EINVAL;

	/* Not supported channel frequency. */
	if ((freq - IEEE80211_FIRST_FREQ) % IEEE80211_CHANNEL_GAP != 0)
		return EINVAL;

	uint32_t tx_control;
	wmi_reg_read(ar9271->htc_device, AR9271_PHY_CCK_TX_CTRL, &tx_control);
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_CCK_TX_CTRL,
	    tx_control & ~AR9271_PHY_CCK_TX_CTRL_JAPAN);

	/* Some magic here. */
	uint32_t synth_ctl;
	wmi_reg_read(ar9271->htc_device, AR9271_PHY_SYNTH_CONTROL, &synth_ctl);
	synth_ctl &= 0xc0000000;
	uint32_t channel_select = (freq * 0x10000) / 15;
	synth_ctl = synth_ctl | (1 << 29) | (1 << 28) | channel_select;

	wmi_reg_write(ar9271->htc_device, AR9271_PHY_SYNTH_CONTROL, synth_ctl);

	ieee80211_report_current_freq(ar9271->ieee80211_dev, freq);

	return EOK;
}

errno_t hw_freq_switch(ar9271_t *ar9271, uint16_t freq)
{
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_RFBUS_KILL, 0x1);

	errno_t rc = hw_read_wait(ar9271, AR9271_PHY_RFBUS_GRANT, 0x1, 0x1);
	if (rc != EOK) {
		usb_log_error("Failed to kill RF bus.\n");
		return rc;
	}

	rc = hw_set_freq(ar9271, freq);
	if (rc != EOK) {
		usb_log_error("Failed to HW set frequency.\n");
		return rc;
	}

	rc = hw_activate_phy(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to activate physical layer.\n");
		return rc;
	}

	udelay(1000);
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_RFBUS_KILL, 0x0);

	rc = hw_noise_floor_calibration(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to do NF calibration.\n");
		return rc;
	}

	return EOK;
}

errno_t hw_set_rx_filter(ar9271_t *ar9271, bool assoc)
{
	uint32_t additional_bits = 0;

	if (assoc)
		additional_bits |= AR9271_RX_FILTER_MYBEACON;
	else
		additional_bits |= AR9271_RX_FILTER_BEACON;

	uint32_t filter_bits = AR9271_RX_FILTER_UNI |
	    AR9271_RX_FILTER_MULTI | AR9271_RX_FILTER_BROAD |
	    additional_bits;

	wmi_reg_write(ar9271->htc_device, AR9271_RX_FILTER, filter_bits);

	return EOK;
}

errno_t hw_set_bssid(ar9271_t *ar9271)
{
	ieee80211_dev_t *ieee80211_dev = ar9271->ieee80211_dev;

	nic_address_t bssid;
	ieee80211_query_bssid(ieee80211_dev, &bssid);

	uint32_t *first_4bytes = (uint32_t *) &bssid.address;
	uint16_t *last_2bytes = (uint16_t *) &bssid.address[4];

	wmi_reg_write(ar9271->htc_device, AR9271_BSSID0,
	    uint32_t_le2host(*first_4bytes));

	wmi_reg_write(ar9271->htc_device, AR9271_BSSID1,
	    uint16_t_le2host(*last_2bytes) |
	    ((ieee80211_get_aid(ieee80211_dev) & 0x3fff) << 16));

	return EOK;
}

errno_t hw_rx_init(ar9271_t *ar9271)
{
	wmi_reg_write(ar9271->htc_device, AR9271_COMMAND,
	    AR9271_COMMAND_RX_ENABLE);

	errno_t rc = hw_set_rx_filter(ar9271, false);
	if (rc != EOK) {
		usb_log_error("Failed to set RX filtering.\n");
		return rc;
	}

	wmi_reg_write(ar9271->htc_device, AR9271_MULTICAST_FILTER1, ~0);
	wmi_reg_write(ar9271->htc_device, AR9271_MULTICAST_FILTER2, ~0);

	/* Disable RX blocking. */
	wmi_reg_clear_bit(ar9271->htc_device, AR9271_DIAG, (0x20 | 0x02000000));

	return EOK;
}

static errno_t hw_init_pll(ar9271_t *ar9271)
{
	/* Some magic here (set for 2GHz channels). But VERY important :-) */
	uint32_t pll = (0x5 << 10) | 0x2c;

	wmi_reg_write(ar9271->htc_device, AR9271_RTC_PLL_CONTROL, pll);

	wmi_reg_write(ar9271->htc_device, AR9271_RTC_SLEEP_CLOCK,
	    AR9271_RTC_SLEEP_CLOCK_FORCE_DERIVED);
	wmi_reg_set_bit(ar9271->htc_device, AR9271_RTC_FORCE_WAKE,
	    AR9271_RTC_FORCE_WAKE_ENABLE);

	return EOK;
}

static void hw_set_init_values(ar9271_t *ar9271)
{
	uint32_t reg_offset;
	uint32_t reg_value;

	size_t size = ARRAY_SIZE(ar9271_2g_mode_array);

	for (size_t i = 0; i < size; i++) {
		reg_offset = ar9271_2g_mode_array[i][0];
		reg_value = ar9271_2g_mode_array[i][1];
		wmi_reg_write(ar9271->htc_device, reg_offset, reg_value);
	}

	size = ARRAY_SIZE(ar9271_2g_tx_array);

	for (size_t i = 0; i < size; i++) {
		reg_offset = ar9271_2g_tx_array[i][0];
		reg_value = ar9271_2g_tx_array[i][1];
		wmi_reg_write(ar9271->htc_device, reg_offset, reg_value);
	}

	size = ARRAY_SIZE(ar9271_init_array);

	for (size_t i = 0; i < size; i++) {
		reg_offset = ar9271_init_array[i][0];
		reg_value = ar9271_init_array[i][1];
		wmi_reg_write(ar9271->htc_device, reg_offset, reg_value);
	}
}

static errno_t hw_calibration(ar9271_t *ar9271)
{
	wmi_reg_set_bit(ar9271->htc_device, AR9271_CARRIER_LEAK_CONTROL,
	    AR9271_CARRIER_LEAK_CALIB);
	wmi_reg_clear_bit(ar9271->htc_device, AR9271_ADC_CONTROL,
	    AR9271_ADC_CONTROL_OFF_PWDADC);
	wmi_reg_set_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_TX_CALIB);
	wmi_reg_set_bit(ar9271->htc_device, AR9271_PHY_TPCRG1,
	    AR9271_PHY_TPCRG1_PD_CALIB);
	wmi_reg_set_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_CALIB);

	errno_t rc = hw_read_wait(ar9271, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_CALIB, 0);
	if (rc != EOK) {
		usb_log_error("Failed to wait on calibrate completion.\n");
		return rc;
	}

	wmi_reg_set_bit(ar9271->htc_device, AR9271_ADC_CONTROL,
	    AR9271_ADC_CONTROL_OFF_PWDADC);
	wmi_reg_clear_bit(ar9271->htc_device, AR9271_CARRIER_LEAK_CONTROL,
	    AR9271_CARRIER_LEAK_CALIB);
	wmi_reg_clear_bit(ar9271->htc_device, AR9271_AGC_CONTROL,
	    AR9271_AGC_CONTROL_TX_CALIB);

	return EOK;
}

errno_t hw_reset(ar9271_t *ar9271)
{
	/* Set physical layer as deactivated. */
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_ACTIVE, 0);

	if (ar9271->starting_up) {
		wmi_reg_write(ar9271->htc_device,
		    AR9271_RESET_POWER_DOWN_CONTROL,
		    AR9271_RADIO_RF_RESET);

		udelay(50);
	}

	/* Cold reset when RX is enabled. */
	uint32_t config_reg;
	wmi_reg_read(ar9271->htc_device, AR9271_COMMAND, &config_reg);
	if (config_reg & AR9271_COMMAND_RX_ENABLE)
		hw_set_reset(ar9271, true);

	errno_t rc = hw_init_pll(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to init PLL.\n");
		return rc;
	}

	udelay(500);

	wmi_reg_write(ar9271->htc_device, AR9271_CLOCK_CONTROL,
	    AR9271_MAX_CPU_CLOCK);

	udelay(100);

	if (ar9271->starting_up) {
		wmi_reg_write(ar9271->htc_device,
		    AR9271_RESET_POWER_DOWN_CONTROL,
		    AR9271_GATE_MAC_CONTROL);

		udelay(50);
	}

	hw_set_init_values(ar9271);

	/* Set physical layer mode. */
	wmi_reg_write(ar9271->htc_device, AR9271_PHY_MODE,
	    AR9271_PHY_MODE_DYNAMIC);

	/* Reset device operating mode. */
	rc = hw_reset_operating_mode(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to reset operating mode.\n");
		return rc;
	}

	/* Set initial channel frequency. */
	rc = hw_set_freq(ar9271, IEEE80211_FIRST_FREQ);
	if (rc != EOK) {
		usb_log_error("Failed to set channel.\n");
		return rc;
	}

	/* Initialize transmission queues. */
	for (unsigned int i = 0; i < AR9271_QUEUES_COUNT; i++) {
		wmi_reg_write(ar9271->htc_device,
		    AR9271_QUEUE_BASE_MASK + (i << 2), 1 << i);
	}

	/* Activate physical layer. */
	rc = hw_activate_phy(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to activate physical layer.\n");
		return rc;
	}

	/* Calibration. */
	rc = hw_calibration(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to calibrate device.\n");
		return rc;
	}

	rc = hw_noise_floor_calibration(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to calibrate noise floor.\n");
		return rc;
	}

	/* Byteswap TX and RX data buffer words. */
	wmi_reg_write(ar9271->htc_device, AR9271_CONFIG, 0xA);

	return EOK;
}

/** Initialize hardware of AR9271 device.
 *
 * @param ar9271 Device structure.
 *
 * @return EOK if succeed, error code otherwise.
 */
errno_t hw_init(ar9271_t *ar9271)
{
	errno_t rc = hw_init_proc(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to HW reset device.\n");
		return rc;
	}

	rc = hw_init_led(ar9271);
	if (rc != EOK) {
		usb_log_error("Failed to HW init led.\n");
		return rc;
	}

	usb_log_info("HW initialization finished successfully.\n");

	return EOK;
}
