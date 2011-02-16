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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_UHCI_H
#define DRV_UHCI_UHCI_H

#include <fibril.h>
#include <fibril_synch.h>
#include <adt/list.h>

#include <usb/addrkeep.h>
#include <usbhc_iface.h>

#include "transfer_list.h"
#include "batch.h"

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
#define UHCI_STATUS_HALTED (1 << 5)
#define UHCI_STATUS_PROCESS_ERROR (1 << 4)
#define UHCI_STATUS_SYSTEM_ERROR (1 << 3)
#define UHCI_STATUS_RESUME (1 << 2)
#define UHCI_STATUS_ERROR_INTERRUPT (1 << 1)
#define UHCI_STATUS_INTERRUPT (1 << 0)

	uint16_t usbintr;
	uint16_t frnum;
	uint32_t flbaseadd;
	uint8_t sofmod;
} regs_t;

#define UHCI_FRAME_LIST_COUNT 1024
#define UHCI_CLEANER_TIMEOUT 10000
#define UHCI_DEBUGER_TIMEOUT 5000000

typedef struct uhci {
	usb_address_keeping_t address_manager;
	volatile regs_t *registers;

	link_pointer_t *frame_list;

	transfer_list_t transfers_bulk_full;
	transfer_list_t transfers_control_full;
	transfer_list_t transfers_control_slow;
	transfer_list_t transfers_interrupt;

	transfer_list_t *transfers[2][4];

	fid_t cleaner;
	fid_t debug_checker;
} uhci_t;

/* init uhci specifics in device.driver_data */
int uhci_init(uhci_t *instance, void *regs, size_t reg_size);

int uhci_fini(uhci_t *device);

int uhci_transfer(
  uhci_t *instance,
  device_t *dev,
  usb_target_t target,
  usb_transfer_type_t transfer_type,
	bool toggle,
  usb_packet_id pid,
	bool low_speed,
  void *buffer, size_t size,
  usbhc_iface_transfer_out_callback_t callback_out,
  usbhc_iface_transfer_in_callback_t callback_in,
  void *arg );

int uhci_schedule(uhci_t *instance, batch_t *batch);

static inline uhci_t * dev_to_uhci(device_t *dev)
	{ return (uhci_t*)dev->driver_data; }

#endif
/**
 * @}
 */
