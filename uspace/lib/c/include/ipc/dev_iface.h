/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_IPC_DEV_IFACE_H_
#define _LIBC_IPC_DEV_IFACE_H_

#include <stdlib.h>
#include <types/common.h>

typedef enum {
	HW_RES_DEV_IFACE = 0,
	PIO_WINDOW_DEV_IFACE,

	/** Character device interface */
	CHAR_DEV_IFACE,

	/** Audio device mixer interface */
	AUDIO_MIXER_IFACE,
	/** Audio device pcm buffer interface */
	AUDIO_PCM_BUFFER_IFACE,

	/** Network interface controller interface */
	NIC_DEV_IFACE,

	/** IEEE 802.11 interface controller interface */
	IEEE80211_DEV_IFACE,

	/** Interface provided by any PCI device. */
	PCI_DEV_IFACE,

	/** Interface provided by any USB device. */
	USB_DEV_IFACE,
	/** Interface provided by USB diagnostic devices. */
	USBDIAG_DEV_IFACE,
	/** Interface provided by USB host controller to USB device. */
	USBHC_DEV_IFACE,
	/** Interface provided by USB HID devices. */
	USBHID_DEV_IFACE,

	/** Interface provided by Real Time Clock devices */
	CLOCK_DEV_IFACE,

	/** Interface provided by LED devices */
	LED_DEV_IFACE,

	/** Interface provided by battery powered devices */
	BATTERY_DEV_IFACE,

	/** Interface provided by AHCI devices. */
	AHCI_DEV_IFACE,

	DEV_IFACE_MAX
} dev_inferface_idx_t;

#define DEV_IFACE_ID(idx)	((idx) + IPC_FIRST_USER_METHOD)
#define DEV_IFACE_IDX(id)	((id) - IPC_FIRST_USER_METHOD)

#define DEV_IFACE_COUNT			DEV_IFACE_MAX
#define DEV_FIRST_CUSTOM_METHOD_IDX	DEV_IFACE_MAX
#define DEV_FIRST_CUSTOM_METHOD \
	DEV_IFACE_ID(DEV_FIRST_CUSTOM_METHOD_IDX)

/*
 * The first argument is actually method (as the "real" method is used
 * for indexing into interfaces.
 */

#define DEV_IPC_GET_ARG1(call) ipc_get_arg2(&(call))
#define DEV_IPC_GET_ARG2(call) ipc_get_arg3(&(call))
#define DEV_IPC_GET_ARG3(call) ipc_get_arg4(&(call))
#define DEV_IPC_GET_ARG4(call) ipc_get_arg5(&(call))

#endif
