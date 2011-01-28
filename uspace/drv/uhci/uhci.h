/*
 * Copyright (c) 2010 Jan Vesely
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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_UHCI_H
#define DRV_UHCI_UHCI_H

#include <fibril.h>

#include <usb/addrkeep.h>
#include <usb/hcdhubd.h>
#include <usbhc_iface.h>

#include "root_hub/root_hub.h"
#include "transfer_list.h"

typedef struct uhci_regs {
	uint16_t usbcmd;
#define UHCI_CMD_MAX_PACKET (1 << 7)
#define UHCI_CMD_CONFIGURE  (1 << 6)
#define UHCI_CMD_DEBUG  (1 << 5)
#define UHCI_CMD_FORCE_GLOBAL_RESUME  (1 << 4)
#define UHCI_CMD_FORCE_GLOBAL_SUSPEND  (1 << 3)
#define UHCI_CMD_GLOBAL_RESET  (1 << 2)
#define UHCI_CMD_HCRESET  (1 << 1)
#define UHCI_CMD_RUN_STOP  (1 << 0)

	uint16_t usbsts;
	uint16_t usbintr;
	uint16_t frnum;
	uint32_t flbaseadd;
	uint8_t sofmod;
} regs_t;

#define TRANSFER_QUEUES 4
#define UHCI_FRAME_LIST_COUNT 1024
#define UHCI_CLEANER_TIMEOUT 5000000
#define UHCI_DEBUGER_TIMEOUT 3000000

typedef struct uhci {
	usb_address_keeping_t address_manager;
	uhci_root_hub_t root_hub;
	volatile regs_t *registers;

	link_pointer_t *frame_list;

	transfer_list_t transfers[TRANSFER_QUEUES];
	fid_t cleaner;
	fid_t debug_checker;
} uhci_t;

/* init uhci specifics in device.driver_data */
int uhci_init( device_t *device, void *regs );

int uhci_destroy( device_t *device );

int uhci_transfer(
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
	bool toggle,
  usb_packet_id pid,
  void *buffer, size_t size,
  usbhc_iface_transfer_out_callback_t callback_out,
  usbhc_iface_transfer_in_callback_t callback_in,
  void *arg );

#endif
/**
 * @}
 */
