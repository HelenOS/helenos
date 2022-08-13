/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup libdrv
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stddef.h>

#include "dev_iface.h"
#include "remote_hw_res.h"
#include "remote_pio_window.h"
#include "remote_clock_dev.h"
#include "remote_led_dev.h"
#include "remote_battery_dev.h"
#include "remote_nic.h"
#include "remote_ieee80211.h"
#include "remote_usb.h"
#include "remote_usbdiag.h"
#include "remote_usbhc.h"
#include "remote_usbhid.h"
#include "remote_pci.h"
#include "remote_audio_mixer.h"
#include "remote_audio_pcm.h"
#include "remote_ahci.h"

static const iface_dipatch_table_t remote_ifaces = {
	.ifaces = {
		[AUDIO_MIXER_IFACE] = &remote_audio_mixer_iface,
		[AUDIO_PCM_BUFFER_IFACE] = &remote_audio_pcm_iface,
		[HW_RES_DEV_IFACE] = &remote_hw_res_iface,
		[PIO_WINDOW_DEV_IFACE] = &remote_pio_window_iface,
		[NIC_DEV_IFACE] = &remote_nic_iface,
		[IEEE80211_DEV_IFACE] = &remote_ieee80211_iface,
		[PCI_DEV_IFACE] = &remote_pci_iface,
		[USB_DEV_IFACE] = &remote_usb_iface,
		[USBDIAG_DEV_IFACE] = &remote_usbdiag_iface,
		[USBHC_DEV_IFACE] = &remote_usbhc_iface,
		[USBHID_DEV_IFACE] = &remote_usbhid_iface,
		[CLOCK_DEV_IFACE] = &remote_clock_dev_iface,
		[LED_DEV_IFACE] = &remote_led_dev_iface,
		[BATTERY_DEV_IFACE] = &remote_battery_dev_iface,
		[AHCI_DEV_IFACE] = &remote_ahci_iface,
	}
};

const remote_iface_t *get_remote_iface(int idx)
{
	assert(is_valid_iface_idx(idx));
	return remote_ifaces.ifaces[idx];
}

remote_iface_func_ptr_t
get_remote_method(const remote_iface_t *rem_iface, sysarg_t iface_method_idx)
{
	if (iface_method_idx >= rem_iface->method_count)
		return NULL;

	return rem_iface->methods[iface_method_idx];
}

bool is_valid_iface_idx(int idx)
{
	return (0 <= idx) && (idx < DEV_IFACE_MAX);
}

/**
 * @}
 */
