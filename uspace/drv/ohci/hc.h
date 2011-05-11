/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI host controller driver structure
 */
#ifndef DRV_OHCI_HC_H
#define DRV_OHCI_HC_H

#include <fibril.h>
#include <fibril_synch.h>
#include <adt/list.h>
#include <ddi.h>

#include <usb/usb.h>
#include <usb/host/device_keeper.h>
#include <usb/host/usb_endpoint_manager.h>
#include <usbhc_iface.h>

#include "batch.h"
#include "ohci_regs.h"
#include "root_hub.h"
#include "endpoint_list.h"
#include "hw_struct/hcca.h"

#define OHCI_NEEDED_IRQ_COMMANDS 5

typedef struct hc {
	ohci_regs_t *registers;
	hcca_t *hcca;

	usb_address_t rh_address;
	rh_t rh;

	endpoint_list_t lists[4];
	link_t pending_batches;

	usb_device_keeper_t manager;
	usb_endpoint_manager_t ep_manager;
	fid_t interrupt_emulator;
	fibril_mutex_t guard;

	/** Code to be executed in kernel interrupt handler */
	irq_code_t interrupt_code;

	/** Commands that form interrupt code */
	irq_cmd_t interrupt_commands[OHCI_NEEDED_IRQ_COMMANDS];
} hc_t;

int hc_register_hub(hc_t *instance, ddf_fun_t *hub_fun);

int hc_init(hc_t *instance, uintptr_t regs, size_t reg_size, bool interrupts);

void hc_start_hw(hc_t *instance);

/** Safely dispose host controller internal structures
 *
 * @param[in] instance Host controller structure to use.
 */
static inline void hc_fini(hc_t *instance) { /* TODO: implement*/ };

int hc_add_endpoint(hc_t *instance, usb_address_t address, usb_endpoint_t ep,
    usb_speed_t speed, usb_transfer_type_t type, usb_direction_t direction,
    size_t max_packet_size, size_t size, unsigned interval);

int hc_remove_endpoint(hc_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction);

endpoint_t * hc_get_endpoint(hc_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction, size_t *bw);

int hc_schedule(hc_t *instance, usb_transfer_batch_t *batch);

void hc_interrupt(hc_t *instance, uint32_t status);

/** Get and cast pointer to the driver data
 *
 * @param[in] fun DDF function pointer
 * @return cast pointer to driver_data
 */
static inline hc_t * fun_to_hc(ddf_fun_t *fun)
	{ return (hc_t*)fun->driver_data; }
#endif
/**
 * @}
 */
