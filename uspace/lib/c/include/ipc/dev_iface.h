/*
 * Copyright (c) 2010 Lenka Trochtova
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

#ifndef LIBC_IPC_DEV_IFACE_H_
#define LIBC_IPC_DEV_IFACE_H_

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

#define DEV_IPC_GET_ARG1(call) IPC_GET_ARG2((call))
#define DEV_IPC_GET_ARG2(call) IPC_GET_ARG3((call))
#define DEV_IPC_GET_ARG3(call) IPC_GET_ARG4((call))
#define DEV_IPC_GET_ARG4(call) IPC_GET_ARG5((call))


#endif
